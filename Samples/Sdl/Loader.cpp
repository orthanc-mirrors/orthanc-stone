/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/

// From Stone
#include "../../Framework/Messages/ICallable.h"
#include "../../Framework/Messages/IMessage.h"
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Messages/MessageBroker.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Toolbox/GeometryToolbox.h"
#include "../../Framework/Volumes/ImageBuffer3D.h"

// From Orthanc framework
#include <Core/DicomFormat/DicomArray.h>
#include <Core/DicomFormat/DicomImageInformation.h>
#include <Core/HttpClient.h>
#include <Core/IDynamicObject.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>
#include <Core/Logging.h>
#include <Core/MultiThreading/SharedMessageQueue.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <list>
#include <stdio.h>



namespace Refactoring
{
  class IOracleCommand : public boost::noncopyable
  {
  public:
    enum Type
    {
      Type_OrthancApi
    };

    virtual ~IOracleCommand()
    {
    }

    virtual Type GetType() const = 0;
  };


  class IMessageEmitter : public boost::noncopyable
  {
  public:
    virtual ~IMessageEmitter()
    {
    }

    virtual void EmitMessage(const OrthancStone::IObserver& observer,
                             const OrthancStone::IMessage& message) = 0;
  };


  class IOracle : public boost::noncopyable
  {
  public:
    virtual ~IOracle()
    {
    }

    virtual void Schedule(const OrthancStone::IObserver& receiver,
                          IOracleCommand* command) = 0;  // Takes ownership
  };




  class OracleCommandWithPayload : public IOracleCommand
  {
  private:
    std::auto_ptr<Orthanc::IDynamicObject>  payload_;

  public:
    void SetPayload(Orthanc::IDynamicObject* payload)
    {
      if (payload == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      else
      {
        payload_.reset(payload);
      }    
    }

    bool HasPayload() const
    {
      return (payload_.get() != NULL);
    }

    const Orthanc::IDynamicObject& GetPayload() const
    {
      if (HasPayload())
      {
        return *payload_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
  };



  typedef std::map<std::string, std::string>  HttpHeaders;

  class OrthancApiOracleCommand : public OracleCommandWithPayload
  {
  public:
    class SuccessMessage : public OrthancStone::OriginMessage<OrthancStone::MessageType_HttpRequestSuccess,   // TODO
                                                              OrthancApiOracleCommand>
    {
    private:
      HttpHeaders   headers_;
      std::string   answer_;

    public:
      SuccessMessage(const OrthancApiOracleCommand& command,
                     const HttpHeaders& answerHeaders,
                     std::string& answer  /* will be swapped to avoid a memcpy() */) :
        OriginMessage(command),
        headers_(answerHeaders),
        answer_(answer)
      {
      }

      const std::string& GetAnswer() const
      {
        return answer_;
      }

      void ParseJsonBody(Json::Value& target) const
      {
        Json::Reader reader;
        if (!reader.parse(answer_, target))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      }

      const HttpHeaders&  GetAnswerHeaders() const
      {
        return headers_;
      }
    };


    class FailureMessage : public OrthancStone::OriginMessage<OrthancStone::MessageType_HttpRequestError,   // TODO
                                                              OrthancApiOracleCommand>
    {
    private:
      Orthanc::HttpStatus  status_;

    public:
      FailureMessage(const OrthancApiOracleCommand& command,
                     Orthanc::HttpStatus status) :
        OriginMessage(command),
        status_(status)
      {
      }

      Orthanc::HttpStatus GetHttpStatus() const
      {
        return status_;
      }
    };


  private:
    Orthanc::HttpMethod  method_;
    std::string          uri_;
    std::string          body_;
    HttpHeaders          headers_;
    unsigned int         timeout_;

    std::auto_ptr< OrthancStone::MessageHandler<SuccessMessage> >  successCallback_;
    std::auto_ptr< OrthancStone::MessageHandler<FailureMessage> >  failureCallback_;

  public:
    OrthancApiOracleCommand() :
      method_(Orthanc::HttpMethod_Get),
      uri_("/"),
      timeout_(10)
    {
    }

    virtual Type GetType() const
    {
      return Type_OrthancApi;
    }

    void SetMethod(Orthanc::HttpMethod method)
    {
      method_ = method;
    }

    void SetUri(const std::string& uri)
    {
      uri_ = uri;
    }

    void SetBody(const std::string& body)
    {
      body_ = body;
    }

    void SetBody(const Json::Value& json)
    {
      Json::FastWriter writer;
      body_ = writer.write(json);
    }

    void SetHttpHeader(const std::string& key,
                       const std::string& value)
    {
      headers_[key] = value;
    }

    Orthanc::HttpMethod GetMethod() const
    {
      return method_;
    }

    const std::string& GetUri() const
    {
      return uri_;
    }

    const std::string& GetBody() const
    {
      if (method_ == Orthanc::HttpMethod_Post ||
          method_ == Orthanc::HttpMethod_Put)
      {
        return body_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    const HttpHeaders& GetHttpHeaders() const
    {
      return headers_;
    }

    void SetTimeout(unsigned int seconds)
    {
      timeout_ = seconds;
    }

    unsigned int GetTimeout() const
    {
      return timeout_;
    }
  };



  class NativeOracle : public IOracle
  {
  private:
    class Item : public Orthanc::IDynamicObject
    {
    private:
      const OrthancStone::IObserver&  receiver_;
      std::auto_ptr<IOracleCommand>   command_;

    public:
      Item(const OrthancStone::IObserver& receiver,
           IOracleCommand* command) :
        receiver_(receiver),
        command_(command)
      {
        if (command == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      const OrthancStone::IObserver& GetReceiver() const
      {
        return receiver_;
      }

      const IOracleCommand& GetCommand() const
      {
        assert(command_.get() != NULL);
        return *command_;
      }
    };


    enum State
    {
      State_Setup,
      State_Running,
      State_Stopped
    };


    IMessageEmitter&               emitter_;
    Orthanc::WebServiceParameters  orthanc_;
    Orthanc::SharedMessageQueue    queue_;
    State                          state_;
    boost::mutex                   mutex_;
    std::vector<boost::thread*>    workers_;


    void Execute(const OrthancStone::IObserver& receiver,
                 const OrthancApiOracleCommand& command)
    {
      Orthanc::HttpClient  client(orthanc_, command.GetUri());
      client.SetMethod(command.GetMethod());
      client.SetTimeout(command.GetTimeout());

      if (command.GetMethod() == Orthanc::HttpMethod_Post ||
          command.GetMethod() == Orthanc::HttpMethod_Put)
      {
        client.SetBody(command.GetBody());
      }
      
      {
        const HttpHeaders& headers = command.GetHttpHeaders();
        for (HttpHeaders::const_iterator it = headers.begin(); it != headers.end(); it++ )
        {
          client.AddHeader(it->first, it->second);
        }
      }

      std::string answer;
      HttpHeaders answerHeaders;

      bool success;
      try
      {
        success = client.Apply(answer, answerHeaders);
      }
      catch (Orthanc::OrthancException& e)
      {
        success = false;
      }

      if (success)
      {
        OrthancApiOracleCommand::SuccessMessage message(command, answerHeaders, answer);
        emitter_.EmitMessage(receiver, message);
      }
      else
      {
        OrthancApiOracleCommand::FailureMessage message(command, client.GetLastStatus());
        emitter_.EmitMessage(receiver, message);
      }
    }



    void Step()
    {
      std::auto_ptr<Orthanc::IDynamicObject>  object(queue_.Dequeue(100));

      if (object.get() != NULL)
      {
        const Item& item = dynamic_cast<Item&>(*object);

        try
        {
          switch (item.GetCommand().GetType())
          {
            case IOracleCommand::Type_OrthancApi:
              Execute(item.GetReceiver(), 
                      dynamic_cast<const OrthancApiOracleCommand&>(item.GetCommand()));
              break;

            default:
              throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
          }
        }
        catch (Orthanc::OrthancException& e)
        {
          LOG(ERROR) << "Exception within the oracle: " << e.What();
        }
        catch (...)
        {
          LOG(ERROR) << "Native exception within the oracle";
        }
      }
    }


    static void Worker(NativeOracle* that)
    {
      assert(that != NULL);
      
      for (;;)
      {
        {
          boost::mutex::scoped_lock lock(that->mutex_);
          if (that->state_ != State_Running)
          {
            return;
          }
        }

        that->Step();
      }
    }


    void StopInternal()
    {
      {
        boost::mutex::scoped_lock lock(mutex_);

        if (state_ == State_Setup ||
            state_ == State_Stopped)
        {
          return;
        }
        else
        {
          state_ = State_Stopped;
        }
      }

      for (size_t i = 0; i < workers_.size(); i++)
      {
        if (workers_[i] != NULL)
        {
          if (workers_[i]->joinable())
          {
            workers_[i]->join();
          }

          delete workers_[i];
        }
      } 
    }


  public:
    NativeOracle(IMessageEmitter& emitter) :
    emitter_(emitter),
      state_(State_Setup),
      workers_(4)
    {
    }

    virtual ~NativeOracle()
    {
      StopInternal();
    }

    void SetOrthancParameters(const Orthanc::WebServiceParameters& orthanc)
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (state_ != State_Setup)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        orthanc_ = orthanc;
      }
    }

    void SetWorkersCount(unsigned int count)
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (count <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else if (state_ != State_Setup)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        workers_.resize(count);
      }
    }

    void Start()
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (state_ != State_Setup)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        state_ = State_Running;

        for (unsigned int i = 0; i < workers_.size(); i++)
        {
          workers_[i] = new boost::thread(Worker, this);
        }
      }      
    }

    void Stop()
    {
      StopInternal();
    }

    virtual void Schedule(const OrthancStone::IObserver& receiver,
                          IOracleCommand* command)
    {
      queue_.Enqueue(new Item(receiver, command));
    }
  };



  class NativeApplicationContext : public IMessageEmitter
  {
  private:
    boost::shared_mutex            mutex_;
    OrthancStone::MessageBroker    broker_;
    OrthancStone::IObservable      oracleObservable_;

  public:
    NativeApplicationContext() :
      oracleObservable_(broker_)
    {
    }


    virtual void EmitMessage(const OrthancStone::IObserver& observer,
                             const OrthancStone::IMessage& message)
    {
      boost::unique_lock<boost::shared_mutex>  lock(mutex_);
      oracleObservable_.EmitMessage(observer, message);
    }


    class ReaderLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::shared_lock<boost::shared_mutex>  lock_;

    public:
      ReaderLock(NativeApplicationContext& that) : 
      that_(that),
      lock_(that.mutex_)
      {
      }
    };


    class WriterLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::unique_lock<boost::shared_mutex>  lock_;

    public:
      WriterLock(NativeApplicationContext& that) : 
      that_(that),
      lock_(that.mutex_)
      {
      }

      OrthancStone::MessageBroker& GetBroker() 
      {
        return that_.broker_;
      }

      OrthancStone::IObservable& GetOracleObservable()
      {
        return that_.oracleObservable_;
      }
    };
  };



  class DicomInstanceParameters : public boost::noncopyable
  {
  private:
    Orthanc::DicomImageInformation    imageInformation_;
    OrthancStone::SopClassUid         sopClassUid_;
    double                            thickness_;
    double                            pixelSpacingX_;
    double                            pixelSpacingY_;
    OrthancStone::CoordinateSystem3D  geometry_;
    OrthancStone::Vector              frameOffsets_;
    bool                              isColor_;
    bool                              hasRescale_;
    double                            rescaleOffset_;
    double                            rescaleSlope_;
    bool                              hasDefaultWindowing_;
    float                             defaultWindowingCenter_;
    float                             defaultWindowingWidth_;
    Orthanc::PixelFormat              expectedPixelFormat_;

    void ComputeDoseOffsets(const Orthanc::DicomMap& dicom)
    {
      // http://dicom.nema.org/medical/Dicom/2016a/output/chtml/part03/sect_C.8.8.3.2.html

      {
        std::string increment;

        if (dicom.CopyToString(increment, Orthanc::DICOM_TAG_FRAME_INCREMENT_POINTER, false))
        {
          Orthanc::Toolbox::ToUpperCase(increment);
          if (increment != "3004,000C")  // This is the "Grid Frame Offset Vector" tag
          {
            LOG(ERROR) << "RT-DOSE: Bad value for the \"FrameIncrementPointer\" tag";
            return;
          }
        }
      }

      if (!OrthancStone::LinearAlgebra::ParseVector(frameOffsets_, dicom, Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR) ||
          frameOffsets_.size() < imageInformation_.GetNumberOfFrames())
      {
        LOG(ERROR) << "RT-DOSE: No information about the 3D location of some slice(s)";
        frameOffsets_.clear();
      }
      else
      {
        if (frameOffsets_.size() >= 2)
        {
          thickness_ = frameOffsets_[1] - frameOffsets_[0];

          if (thickness_ < 0)
          {
            thickness_ = -thickness_;
          }
        }
      }
    }

  public:
    DicomInstanceParameters(const Orthanc::DicomMap& dicom) :
      imageInformation_(dicom)
    {
      if (imageInformation_.GetNumberOfFrames() <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
            
      std::string s;
      if (!dicom.CopyToString(s, Orthanc::DICOM_TAG_SOP_CLASS_UID, false))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else
      {
        sopClassUid_ = OrthancStone::StringToSopClassUid(s);
      }

      if (!dicom.ParseDouble(thickness_, Orthanc::DICOM_TAG_SLICE_THICKNESS))
      {
        thickness_ = 100.0 * std::numeric_limits<double>::epsilon();
      }

      OrthancStone::GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, dicom);

      std::string position, orientation;
      if (dicom.CopyToString(position, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT, false) &&
          dicom.CopyToString(orientation, Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT, false))
      {
        geometry_ = OrthancStone::CoordinateSystem3D(position, orientation);
      }

      if (sopClassUid_ == OrthancStone::SopClassUid_RTDose)
      {
        ComputeDoseOffsets(dicom);
      }

      isColor_ = (imageInformation_.GetPhotometricInterpretation() != Orthanc::PhotometricInterpretation_Monochrome1 &&
                  imageInformation_.GetPhotometricInterpretation() != Orthanc::PhotometricInterpretation_Monochrome2);

      double doseGridScaling;

      if (dicom.ParseDouble(rescaleOffset_, Orthanc::DICOM_TAG_RESCALE_INTERCEPT) &&
          dicom.ParseDouble(rescaleSlope_, Orthanc::DICOM_TAG_RESCALE_SLOPE))
      {
        hasRescale_ = true;
      }
      else if (dicom.ParseDouble(doseGridScaling, Orthanc::DICOM_TAG_DOSE_GRID_SCALING))
      {
        hasRescale_ = true;
        rescaleOffset_ = 0;
        rescaleSlope_ = doseGridScaling;
      }
      else
      {
        hasRescale_ = false;
      }

      OrthancStone::Vector c, w;
      if (OrthancStone::LinearAlgebra::ParseVector(c, dicom, Orthanc::DICOM_TAG_WINDOW_CENTER) &&
          OrthancStone::LinearAlgebra::ParseVector(w, dicom, Orthanc::DICOM_TAG_WINDOW_WIDTH) &&
          c.size() > 0 && 
          w.size() > 0)
      {
        hasDefaultWindowing_ = true;
        defaultWindowingCenter_ = static_cast<float>(c[0]);
        defaultWindowingWidth_ = static_cast<float>(w[0]);
      }
      else
      {
        hasDefaultWindowing_ = false;
      }

      if (sopClassUid_ == OrthancStone::SopClassUid_RTDose)
      {
        switch (imageInformation_.GetBitsStored())
        {
          case 16:
            expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale16;
            break;

          case 32:
            expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale32;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        } 
      }
      else if (isColor_)
      {
        expectedPixelFormat_ = Orthanc::PixelFormat_RGB24;
      }
      else if (imageInformation_.IsSigned())
      {
        expectedPixelFormat_ = Orthanc::PixelFormat_SignedGrayscale16;
      }
      else
      {
        expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale16;
      }
    }

    const Orthanc::DicomImageInformation& GetImageInformation() const
    {
      return imageInformation_;
    }

    OrthancStone::SopClassUid GetSopClassUid() const
    {
      return sopClassUid_;
    }

    double GetThickness() const
    {
      return thickness_;
    }

    double GetPixelSpacingX() const
    {
      return pixelSpacingX_;
    }

    double GetPixelSpacingY() const
    {
      return pixelSpacingY_;
    }

    const OrthancStone::CoordinateSystem3D&  GetGeometry() const
    {
      return geometry_;
    }

    OrthancStone::CoordinateSystem3D  GetFrameGeometry(unsigned int frame) const
    {
      if (frame >= imageInformation_.GetNumberOfFrames())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      if (sopClassUid_ == OrthancStone::SopClassUid_RTDose)
      {
        if (frame >= frameOffsets_.size())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        return OrthancStone::CoordinateSystem3D(
          geometry_.GetOrigin() + frameOffsets_[frame] * geometry_.GetNormal(),
          geometry_.GetAxisX(),
          geometry_.GetAxisY());
      }
    }

    bool FrameContainsPlane(unsigned int frame,
                            const OrthancStone::CoordinateSystem3D& plane) const
    {
      if (frame >= imageInformation_.GetNumberOfFrames())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      OrthancStone::CoordinateSystem3D tmp = geometry_;

      if (frame != 0)
      {
        tmp = GetFrameGeometry(frame);
      }

      bool opposite;   // Ignored
      return (OrthancStone::GeometryToolbox::IsParallelOrOpposite(
                opposite, tmp.GetNormal(), plane.GetNormal()) &&
              OrthancStone::LinearAlgebra::IsNear(
                tmp.ProjectAlongNormal(tmp.GetOrigin()),
                tmp.ProjectAlongNormal(plane.GetOrigin()),
                thickness_ / 2.0));
    }

    bool IsColor() const
    {
      return isColor_;
    }

    bool HasRescale() const
    {
      return hasRescale_;
    }

    double GetRescaleOffset() const
    {
      if (hasRescale_)
      {
        return rescaleOffset_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    double GetRescaleSlope() const
    {
      if (hasRescale_)
      {
        return rescaleSlope_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    bool HasDefaultWindowing() const
    {
      return hasDefaultWindowing_;
    }

    float GetDefaultWindowingCenter() const
    {
      if (hasDefaultWindowing_)
      {
        return defaultWindowingCenter_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    float GetDefaultWindowingWidth() const
    {
      if (hasDefaultWindowing_)
      {
        return defaultWindowingWidth_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    Orthanc::PixelFormat GetExpectedPixelFormat() const
    {
      return expectedPixelFormat_;
    }
  };


  class AxialVolumeOrthancLoader : public OrthancStone::IObserver
  {
  private:
    class MessageHandler : public Orthanc::IDynamicObject
    {
    public:
      virtual void Handle(const OrthancApiOracleCommand::SuccessMessage& message) const = 0;
    };

    void Handle(const OrthancApiOracleCommand::SuccessMessage& message)
    {
      dynamic_cast<const MessageHandler&>(message.GetOrigin().GetPayload()).Handle(message);
    }


    class LoadSeriesGeometryHandler : public MessageHandler
    {
    private:
      AxialVolumeOrthancLoader&  that_;

    public:
      LoadSeriesGeometryHandler(AxialVolumeOrthancLoader& that) :
      that_(that)
      {
      }

      virtual void Handle(const OrthancApiOracleCommand::SuccessMessage& message) const
      {
        Json::Value value;
        message.ParseJsonBody(value);

        if (value.type() != Json::objectValue)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }

        Json::Value::Members instances = value.getMemberNames();

        for (size_t i = 0; i < instances.size(); i++)
        {
          Orthanc::DicomMap dicom;
          dicom.FromDicomAsJson(value[instances[i]]);

          DicomInstanceParameters instance(dicom);
        }
      }
    };


    class LoadInstanceGeometryHandler : public MessageHandler
    {
    private:
      AxialVolumeOrthancLoader&  that_;

    public:
      LoadInstanceGeometryHandler(AxialVolumeOrthancLoader& that) :
      that_(that)
      {
      }

      virtual void Handle(const OrthancApiOracleCommand::SuccessMessage& message) const
      {
        Json::Value value;
        message.ParseJsonBody(value);

        if (value.type() != Json::objectValue)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }

        Orthanc::DicomMap dicom;
        dicom.FromDicomAsJson(value);

        DicomInstanceParameters instance(dicom);
      }
    };


    bool                                        active_;
    std::auto_ptr<OrthancStone::ImageBuffer3D>  image_;


  public:
    AxialVolumeOrthancLoader(OrthancStone::IObservable& oracle) :
      IObserver(oracle.GetBroker()),
      active_(false)
    {
      oracle.RegisterObserverCallback(
        new OrthancStone::Callable<AxialVolumeOrthancLoader, OrthancApiOracleCommand::SuccessMessage>
        (*this, &AxialVolumeOrthancLoader::Handle));
    }

    void LoadSeries(IOracle& oracle,
                    const std::string& seriesId)
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      active_ = true;

      std::auto_ptr<Refactoring::OrthancApiOracleCommand> command(new Refactoring::OrthancApiOracleCommand);
      command->SetUri("/series/" + seriesId + "/instances-tags");
      command->SetPayload(new LoadSeriesGeometryHandler(*this));

      oracle.Schedule(*this, command.release());
    }

    void LoadInstance(IOracle& oracle,
                      const std::string& instanceId)
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      active_ = true;

      // Tag "3004-000c" is "Grid Frame Offset Vector", which is
      // mandatory to read RT DOSE, but is too long to be returned by default

      // TODO => Should be part of a second call if needed

      std::auto_ptr<Refactoring::OrthancApiOracleCommand> command(new Refactoring::OrthancApiOracleCommand);
      command->SetUri("/instances/" + instanceId + "/tags?ignore-length=3004-000c");
      command->SetPayload(new LoadInstanceGeometryHandler(*this));

      oracle.Schedule(*this, command.release());
    }
  };

}



class Toto : public OrthancStone::IObserver
{
private:
  void Handle(const Refactoring::OrthancApiOracleCommand::SuccessMessage& message)
  {
    Json::Value v;
    message.ParseJsonBody(v);

    printf("ICI [%s]\n", v.toStyledString().c_str());
  }

  void Handle(const Refactoring::OrthancApiOracleCommand::FailureMessage& message)
  {
    printf("ERROR %d\n", message.GetHttpStatus());
  }

public:
  Toto(OrthancStone::IObservable& oracle) :
    IObserver(oracle.GetBroker())
  {
    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, Refactoring::OrthancApiOracleCommand::SuccessMessage>(*this, &Toto::Handle));
  }
};


void Run(Refactoring::NativeApplicationContext& context)
{
  std::auto_ptr<Toto> toto;
  std::auto_ptr<Refactoring::AxialVolumeOrthancLoader> loader1, loader2;

  {
    Refactoring::NativeApplicationContext::WriterLock lock(context);
    toto.reset(new Toto(lock.GetOracleObservable()));
    loader1.reset(new Refactoring::AxialVolumeOrthancLoader(lock.GetOracleObservable()));
    loader2.reset(new Refactoring::AxialVolumeOrthancLoader(lock.GetOracleObservable()));
  }

  Refactoring::NativeOracle oracle(context);

  {
    Orthanc::WebServiceParameters p;
    //p.SetUrl("http://localhost:8043/");
    p.SetCredentials("orthanc", "orthanc");
    oracle.SetOrthancParameters(p);
  }

  oracle.Start();

  {
    Json::Value v = Json::objectValue;
    v["Level"] = "Series";
    v["Query"] = Json::objectValue;

    std::auto_ptr<Refactoring::OrthancApiOracleCommand>  command(new Refactoring::OrthancApiOracleCommand);
    command->SetMethod(Orthanc::HttpMethod_Post);
    command->SetUri("/tools/find");
    command->SetBody(v);

    oracle.Schedule(*toto, command.release());
  }
  
  // 2017-11-17-Anonymized
  loader1->LoadSeries(oracle, "cb3ea4d1-d08f3856-ad7b6314-74d88d77-60b05618");  // CT
  loader2->LoadInstance(oracle, "41029085-71718346-811efac4-420e2c15-d39f99b6");  // RT-DOSE

  boost::this_thread::sleep(boost::posix_time::seconds(1));

  oracle.Stop();
}



/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  OrthancStone::StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);

  try
  {
    Refactoring::NativeApplicationContext context;
    Run(context);
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::StoneFinalize();

  return 0;
}
