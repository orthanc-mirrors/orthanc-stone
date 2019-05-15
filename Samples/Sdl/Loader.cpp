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
#include "../../Framework/Toolbox/SlicesSorter.h"

// From Orthanc framework
#include <Core/Compression/GzipCompressor.h>
#include <Core/Compression/ZlibCompressor.h>
#include <Core/DicomFormat/DicomArray.h>
#include <Core/DicomFormat/DicomImageInformation.h>
#include <Core/HttpClient.h>
#include <Core/IDynamicObject.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/JpegReader.h>
#include <Core/Images/PamReader.h>
#include <Core/Images/PngReader.h>
#include <Core/Images/PngWriter.h>
#include <Core/Logging.h>
#include <Core/MultiThreading/SharedMessageQueue.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Core/SystemToolbox.h>

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
      Type_OrthancRestApi,
      Type_GetOrthancImage,
      Type_GetOrthancWebViewerJpeg
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



  class OracleCommandExceptionMessage : public OrthancStone::IMessage
  {
    ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

  private:
    const IOracleCommand&       command_;
    Orthanc::OrthancException   exception_;

  public:
    OracleCommandExceptionMessage(const IOracleCommand& command,
                                  const Orthanc::OrthancException& exception) :
      command_(command),
      exception_(exception)
    {
    }

    OracleCommandExceptionMessage(const IOracleCommand& command,
                                  const Orthanc::ErrorCode& error) :
      command_(command),
      exception_(error)
    {
    }

    const IOracleCommand& GetCommand() const
    {
      return command_;
    }
    
    const Orthanc::OrthancException& GetException() const
    {
      return exception_;
    }
  };
  

  typedef std::map<std::string, std::string>  HttpHeaders;

  class OrthancRestApiCommand : public OracleCommandWithPayload
  {
  public:
    class SuccessMessage : public OrthancStone::OriginMessage<OrthancRestApiCommand>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      HttpHeaders   headers_;
      std::string   answer_;

    public:
      SuccessMessage(const OrthancRestApiCommand& command,
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


  private:
    Orthanc::HttpMethod  method_;
    std::string          uri_;
    std::string          body_;
    HttpHeaders          headers_;
    unsigned int         timeout_;

    std::auto_ptr< OrthancStone::MessageHandler<SuccessMessage> >  successCallback_;
    std::auto_ptr< OrthancStone::MessageHandler<OracleCommandExceptionMessage> >  failureCallback_;

  public:
    OrthancRestApiCommand() :
      method_(Orthanc::HttpMethod_Get),
      uri_("/"),
      timeout_(10)
    {
    }

    virtual Type GetType() const
    {
      return Type_OrthancRestApi;
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

    void SetHttpHeaders(const HttpHeaders& headers)
    {
      headers_ = headers;
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




  class GetOrthancImageCommand : public OracleCommandWithPayload
  {
  public:
    class SuccessMessage : public OrthancStone::OriginMessage<GetOrthancImageCommand>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      std::auto_ptr<Orthanc::ImageAccessor>  image_;
      Orthanc::MimeType                      mime_;

    public:
      SuccessMessage(const GetOrthancImageCommand& command,
                     Orthanc::ImageAccessor* image,   // Takes ownership
                     Orthanc::MimeType mime) :
        OriginMessage(command),
        image_(image),
        mime_(mime)
      {
        if (image == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      const Orthanc::ImageAccessor& GetImage() const
      {
        return *image_;
      }

      Orthanc::MimeType GetMimeType() const
      {
        return mime_;
      }
    };


  private:
    std::string    uri_;
    HttpHeaders    headers_;
    unsigned int   timeout_;

    std::auto_ptr< OrthancStone::MessageHandler<SuccessMessage> >  successCallback_;
    std::auto_ptr< OrthancStone::MessageHandler<OracleCommandExceptionMessage> >  failureCallback_;

  public:
    GetOrthancImageCommand() :
      uri_("/"),
      timeout_(10)
    {
    }

    virtual Type GetType() const
    {
      return Type_GetOrthancImage;
    }

    void SetUri(const std::string& uri)
    {
      uri_ = uri;
    }

    void SetHttpHeader(const std::string& key,
                       const std::string& value)
    {
      headers_[key] = value;
    }

    const std::string& GetUri() const
    {
      return uri_;
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

    void ProcessHttpAnswer(IMessageEmitter& emitter,
                           const OrthancStone::IObserver& receiver,
                           const std::string& answer,
                           const HttpHeaders& answerHeaders) const
    {
      Orthanc::MimeType contentType = Orthanc::MimeType_Binary;

      for (HttpHeaders::const_iterator it = answerHeaders.begin(); 
           it != answerHeaders.end(); ++it)
      {
        std::string s;
        Orthanc::Toolbox::ToLowerCase(s, it->first);

        if (s == "content-type")
        {
          contentType = Orthanc::StringToMimeType(it->second);
          break;
        }
      }

      std::auto_ptr<Orthanc::ImageAccessor> image;

      switch (contentType)
      {
        case Orthanc::MimeType_Png:
        {
          image.reset(new Orthanc::PngReader);
          dynamic_cast<Orthanc::PngReader&>(*image).ReadFromMemory(answer);
          break;
        }

        case Orthanc::MimeType_Pam:
        {
          image.reset(new Orthanc::PamReader);
          dynamic_cast<Orthanc::PamReader&>(*image).ReadFromMemory(answer);
          break;
        }

        case Orthanc::MimeType_Jpeg:
        {
          image.reset(new Orthanc::JpegReader);
          dynamic_cast<Orthanc::JpegReader&>(*image).ReadFromMemory(answer);
          break;
        }

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol,
                                          "Unsupported HTTP Content-Type for an image: " + 
                                          std::string(Orthanc::EnumerationToString(contentType)));
      }

      SuccessMessage message(*this, image.release(), contentType);
      emitter.EmitMessage(receiver, message);
    }
  };



  class GetOrthancWebViewerJpegCommand : public OracleCommandWithPayload
  {
  public:
    class SuccessMessage : public OrthancStone::OriginMessage<GetOrthancWebViewerJpegCommand>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      std::auto_ptr<Orthanc::ImageAccessor>  image_;

    public:
      SuccessMessage(const GetOrthancWebViewerJpegCommand& command,
                     Orthanc::ImageAccessor* image) :   // Takes ownership
        OriginMessage(command),
        image_(image)
      {
        if (image == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      const Orthanc::ImageAccessor& GetImage() const
      {
        return *image_;
      }
    };

  private:
    std::string           instanceId_;
    unsigned int          frame_;
    unsigned int          quality_;
    HttpHeaders           headers_;
    unsigned int          timeout_;
    Orthanc::PixelFormat  expectedFormat_;

    std::auto_ptr< OrthancStone::MessageHandler<SuccessMessage> >  successCallback_;
    std::auto_ptr< OrthancStone::MessageHandler<OracleCommandExceptionMessage> >  failureCallback_;

  public:
    GetOrthancWebViewerJpegCommand() :
      frame_(0),
      quality_(95),
      timeout_(10),
      expectedFormat_(Orthanc::PixelFormat_Grayscale8)
    {
    }

    virtual Type GetType() const
    {
      return Type_GetOrthancWebViewerJpeg;
    }

    void SetExpectedFormat(Orthanc::PixelFormat format)
    {
      expectedFormat_ = format;
    }

    void SetInstance(const std::string& instanceId)
    {
      instanceId_ = instanceId;
    }

    void SetFrame(unsigned int frame)
    {
      frame_ = frame;
    }

    void SetQuality(unsigned int quality)
    {
      if (quality <= 0 ||
          quality > 100)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        quality_ = quality;
      }
    }

    void SetHttpHeader(const std::string& key,
                       const std::string& value)
    {
      headers_[key] = value;
    }

    Orthanc::PixelFormat GetExpectedFormat() const
    {
      return expectedFormat_;
    }

    const std::string& GetInstanceId() const
    {
      return instanceId_;
    }

    unsigned int GetFrame() const
    {
      return frame_;
    }

    unsigned int GetQuality() const
    {
      return quality_;
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

    std::string GetUri() const
    {
      return ("/web-viewer/instances/jpeg" + boost::lexical_cast<std::string>(quality_) +
              "-" + instanceId_ + "_" + boost::lexical_cast<std::string>(frame_));
    }

    void ProcessHttpAnswer(IMessageEmitter& emitter,
                           const OrthancStone::IObserver& receiver,
                           const std::string& answer) const
    {
      // This code comes from older "OrthancSlicesLoader::ParseSliceImageJpeg()"
      
      Json::Value encoded;

      {
        Json::Reader reader;
        if (!reader.parse(answer, encoded))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      }

      if (encoded.type() != Json::objectValue ||
          !encoded.isMember("Orthanc") ||
          encoded["Orthanc"].type() != Json::objectValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    
      const Json::Value& info = encoded["Orthanc"];
      if (!info.isMember("PixelData") ||
          !info.isMember("Stretched") ||
          !info.isMember("Compression") ||
          info["Compression"].type() != Json::stringValue ||
          info["PixelData"].type() != Json::stringValue ||
          info["Stretched"].type() != Json::booleanValue ||
          info["Compression"].asString() != "Jpeg")
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    
      bool isSigned = false;
      bool isStretched = info["Stretched"].asBool();
    
      if (info.isMember("IsSigned"))
      {
        if (info["IsSigned"].type() != Json::booleanValue)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          isSigned = info["IsSigned"].asBool();
        }
      }
    
      std::auto_ptr<Orthanc::ImageAccessor> reader;
    
      {
        std::string jpeg;
        Orthanc::Toolbox::DecodeBase64(jpeg, info["PixelData"].asString());
      
        reader.reset(new Orthanc::JpegReader);
        dynamic_cast<Orthanc::JpegReader&>(*reader).ReadFromMemory(jpeg);
      }
    
      if (reader->GetFormat() == Orthanc::PixelFormat_RGB24)  // This is a color image
      {
        if (expectedFormat_ != Orthanc::PixelFormat_RGB24)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      
        if (isSigned || isStretched)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          SuccessMessage message(*this, reader.release());
          emitter.EmitMessage(receiver, message);
          return;
        }
      }
    
      if (reader->GetFormat() != Orthanc::PixelFormat_Grayscale8)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    
      if (!isStretched)
      {
        if (expectedFormat_ != reader->GetFormat())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          SuccessMessage message(*this, reader.release());
          emitter.EmitMessage(receiver, message);
          return;
        }
      }
    
      int32_t stretchLow = 0;
      int32_t stretchHigh = 0;
    
      if (!info.isMember("StretchLow") ||
          !info.isMember("StretchHigh") ||
          info["StretchLow"].type() != Json::intValue ||
          info["StretchHigh"].type() != Json::intValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    
      stretchLow = info["StretchLow"].asInt();
      stretchHigh = info["StretchHigh"].asInt();
    
      if (stretchLow < -32768 ||
          stretchHigh > 65535 ||
          (stretchLow < 0 && stretchHigh > 32767))
      {
        // This range cannot be represented with a uint16_t or an int16_t
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    
      // Decode a grayscale JPEG 8bpp image coming from the Web viewer
      std::auto_ptr<Orthanc::ImageAccessor> image
        (new Orthanc::Image(expectedFormat_, reader->GetWidth(), reader->GetHeight(), false));

      Orthanc::ImageProcessing::Convert(*image, *reader);
      reader.reset();
    
      float scaling = static_cast<float>(stretchHigh - stretchLow) / 255.0f;
    
      if (!OrthancStone::LinearAlgebra::IsCloseToZero(scaling))
      {
        float offset = static_cast<float>(stretchLow) / scaling;
        Orthanc::ImageProcessing::ShiftScale(*image, offset, scaling, true);
      }
    
      SuccessMessage message(*this, image.release());
      emitter.EmitMessage(receiver, message);
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


    void CopyHttpHeaders(Orthanc::HttpClient& client,
                         const HttpHeaders& headers)
    {
      for (HttpHeaders::const_iterator it = headers.begin(); it != headers.end(); it++ )
      {
        client.AddHeader(it->first, it->second);
      }
    }


    void DecodeAnswer(std::string& answer,
                      const HttpHeaders& headers)
    {
      Orthanc::HttpCompression contentEncoding = Orthanc::HttpCompression_None;

      for (HttpHeaders::const_iterator it = headers.begin(); 
           it != headers.end(); ++it)
      {
        std::string s;
        Orthanc::Toolbox::ToLowerCase(s, it->first);

        if (s == "content-encoding")
        {
          if (it->second == "gzip")
          {
            contentEncoding = Orthanc::HttpCompression_Gzip;
          }
          else 
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol,
                                            "Unsupported HTTP Content-Encoding: " + it->second);
          }

          break;
        }
      }

      if (contentEncoding == Orthanc::HttpCompression_Gzip)
      {
        std::string compressed;
        answer.swap(compressed);
          
        Orthanc::GzipCompressor compressor;
        compressor.Uncompress(answer, compressed.c_str(), compressed.size());
      }
    }


    void Execute(const OrthancStone::IObserver& receiver,
                 const OrthancRestApiCommand& command)
    {
      Orthanc::HttpClient client(orthanc_, command.GetUri());
      client.SetMethod(command.GetMethod());
      client.SetTimeout(command.GetTimeout());

      CopyHttpHeaders(client, command.GetHttpHeaders());

      if (command.GetMethod() == Orthanc::HttpMethod_Post ||
          command.GetMethod() == Orthanc::HttpMethod_Put)
      {
        client.SetBody(command.GetBody());
      }

      std::string answer;
      HttpHeaders answerHeaders;
      client.ApplyAndThrowException(answer, answerHeaders);

      DecodeAnswer(answer, answerHeaders);

      OrthancRestApiCommand::SuccessMessage message(command, answerHeaders, answer);
      emitter_.EmitMessage(receiver, message);
    }


    void Execute(const OrthancStone::IObserver& receiver,
                 const GetOrthancImageCommand& command)
    {
      Orthanc::HttpClient client(orthanc_, command.GetUri());
      client.SetTimeout(command.GetTimeout());

      CopyHttpHeaders(client, command.GetHttpHeaders());

      std::string answer;
      HttpHeaders answerHeaders;
      client.ApplyAndThrowException(answer, answerHeaders);

      DecodeAnswer(answer, answerHeaders);

      command.ProcessHttpAnswer(emitter_, receiver, answer, answerHeaders);
    }


    void Execute(const OrthancStone::IObserver& receiver,
                 const GetOrthancWebViewerJpegCommand& command)
    {
      Orthanc::HttpClient client(orthanc_, command.GetUri());
      client.SetTimeout(command.GetTimeout());

      CopyHttpHeaders(client, command.GetHttpHeaders());

      std::string answer;
      HttpHeaders answerHeaders;
      client.ApplyAndThrowException(answer, answerHeaders);

      DecodeAnswer(answer, answerHeaders);

      command.ProcessHttpAnswer(emitter_, receiver, answer);
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
            case IOracleCommand::Type_OrthancRestApi:
              Execute(item.GetReceiver(), 
                      dynamic_cast<const OrthancRestApiCommand&>(item.GetCommand()));
              break;

            case IOracleCommand::Type_GetOrthancImage:
              Execute(item.GetReceiver(), 
                      dynamic_cast<const GetOrthancImageCommand&>(item.GetCommand()));
              break;

            case IOracleCommand::Type_GetOrthancWebViewerJpeg:
              Execute(item.GetReceiver(), 
                      dynamic_cast<const GetOrthancWebViewerJpegCommand&>(item.GetCommand()));
              break;

            default:
              throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
          }
        }
        catch (Orthanc::OrthancException& e)
        {
          LOG(ERROR) << "Exception within the oracle: " << e.What();
          emitter_.EmitMessage(item.GetReceiver(), OracleCommandExceptionMessage(item.GetCommand(), e));
        }
        catch (...)
        {
          LOG(ERROR) << "Native exception within the oracle";
          emitter_.EmitMessage(item.GetReceiver(), OracleCommandExceptionMessage
                               (item.GetCommand(), Orthanc::ErrorCode_InternalError));
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
      try
      {
        boost::unique_lock<boost::shared_mutex>  lock(mutex_);
        oracleObservable_.EmitMessage(observer, message);
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Exception while emitting a message: " << e.What();
      }
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



  class DicomInstanceParameters :
    public Orthanc::IDynamicObject  /* to be used as a payload of SlicesSorter */
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
      if (frame == 0)
      {
        return geometry_;
      }
      else if (frame >= imageInformation_.GetNumberOfFrames())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else if (sopClassUid_ == OrthancStone::SopClassUid_RTDose)
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
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
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

      double distance;

      return (OrthancStone::CoordinateSystem3D::GetDistance(distance, tmp, plane) &&
              distance <= thickness_ / 2.0);
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
      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const = 0;
    };

    void Handle(const OrthancRestApiCommand::SuccessMessage& message)
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

      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const
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

          std::auto_ptr<DicomInstanceParameters> instance(new DicomInstanceParameters(dicom));

          OrthancStone::CoordinateSystem3D geometry = instance->GetGeometry();
          that_.slices_.AddSlice(geometry, instance.release());
        }

        if (!that_.slices_.Sort())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                          "Cannot sort the 3D slices of a DICOM series");          
        }

        printf("series sorted %d => %d\n", instances.size(), that_.slices_.GetSlicesCount());
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

      virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message) const
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
    OrthancStone::SlicesSorter                  slices_;

  public:
    AxialVolumeOrthancLoader(OrthancStone::IObservable& oracle) :
      IObserver(oracle.GetBroker()),
      active_(false)
    {
      oracle.RegisterObserverCallback(
        new OrthancStone::Callable<AxialVolumeOrthancLoader, OrthancRestApiCommand::SuccessMessage>
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

      std::auto_ptr<Refactoring::OrthancRestApiCommand> command(new Refactoring::OrthancRestApiCommand);
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

      std::auto_ptr<Refactoring::OrthancRestApiCommand> command(new Refactoring::OrthancRestApiCommand);
      command->SetUri("/instances/" + instanceId + "/tags?ignore-length=3004-000c");
      command->SetPayload(new LoadInstanceGeometryHandler(*this));

      oracle.Schedule(*this, command.release());
    }
  };

}



class Toto : public OrthancStone::IObserver
{
private:
  void Handle(const Refactoring::OrthancRestApiCommand::SuccessMessage& message)
  {
    Json::Value v;
    message.ParseJsonBody(v);

    printf("ICI [%s]\n", v.toStyledString().c_str());
  }

  void Handle(const Refactoring::GetOrthancImageCommand::SuccessMessage& message)
  {
    printf("IMAGE %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const Refactoring::GetOrthancWebViewerJpegCommand::SuccessMessage& message)
  {
    printf("WebViewer %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const Refactoring::OracleCommandExceptionMessage& message)
  {
    printf("EXCEPTION: [%s] on command type %d\n", message.GetException().What(), message.GetCommand().GetType());

    switch (message.GetCommand().GetType())
    {
      case Refactoring::IOracleCommand::Type_GetOrthancWebViewerJpeg:
        printf("URI: [%s]\n", dynamic_cast<const Refactoring::GetOrthancWebViewerJpegCommand&>
               (message.GetCommand()).GetUri().c_str());
        break;
      
      default:
        break;
    }
  }

public:
  Toto(OrthancStone::IObservable& oracle) :
    IObserver(oracle.GetBroker())
  {
    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, Refactoring::OrthancRestApiCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, Refactoring::GetOrthancImageCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
      <Toto, Refactoring::GetOrthancWebViewerJpegCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, Refactoring::OracleCommandExceptionMessage>(*this, &Toto::Handle));
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

  if (1)
  {
    Json::Value v = Json::objectValue;
    v["Level"] = "Series";
    v["Query"] = Json::objectValue;

    std::auto_ptr<Refactoring::OrthancRestApiCommand>  command(new Refactoring::OrthancRestApiCommand);
    command->SetMethod(Orthanc::HttpMethod_Post);
    command->SetUri("/tools/find");
    command->SetBody(v);

    oracle.Schedule(*toto, command.release());
  }
  
  if (1)
  {
    std::auto_ptr<Refactoring::GetOrthancImageCommand>  command(new Refactoring::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Jpeg)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
    oracle.Schedule(*toto, command.release());
  }
  
  if (1)
  {
    std::auto_ptr<Refactoring::GetOrthancImageCommand>  command(new Refactoring::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
    oracle.Schedule(*toto, command.release());
  }
  
  if (1)
  {
    std::auto_ptr<Refactoring::GetOrthancImageCommand>  command(new Refactoring::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }
  
  if (1)
  {
    std::auto_ptr<Refactoring::GetOrthancImageCommand>  command(new Refactoring::GetOrthancImageCommand);
    command->SetHttpHeader("Accept-Encoding", "gzip");
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }
  
  if (1)
  {
    std::auto_ptr<Refactoring::GetOrthancImageCommand>  command(new Refactoring::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }

  if (1)
  {
    std::auto_ptr<Refactoring::GetOrthancWebViewerJpegCommand>  command(new Refactoring::GetOrthancWebViewerJpegCommand);
    command->SetHttpHeader("Accept-Encoding", "gzip");
    command->SetInstance("e6c7c20b-c9f65d7e-0d76f2e2-830186f2-3e3c600e");
    command->SetQuality(90);
    oracle.Schedule(*toto, command.release());
  }


  // 2017-11-17-Anonymized
  //loader1->LoadSeries(oracle, "cb3ea4d1-d08f3856-ad7b6314-74d88d77-60b05618");  // CT
  loader2->LoadInstance(oracle, "41029085-71718346-811efac4-420e2c15-d39f99b6");  // RT-DOSE

  // Delphine
  loader1->LoadSeries(oracle, "5990e39c-51e5f201-fe87a54c-31a55943-e59ef80e");  // CT

  LOG(WARNING) << "...Waiting for Ctrl-C...";
  Orthanc::SystemToolbox::ServerBarrier();
  //boost::this_thread::sleep(boost::posix_time::seconds(1));

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
