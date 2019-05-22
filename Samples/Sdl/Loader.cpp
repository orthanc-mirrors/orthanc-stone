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


#include "../../Framework/Oracle/SleepOracleCommand.h"
#include "../../Framework/Oracle/OracleCommandExceptionMessage.h"
#include "../../Framework/Messages/IMessageEmitter.h"
#include "../../Framework/Oracle/OracleCommandWithPayload.h"
#include "../../Framework/Oracle/IOracle.h"

// From Stone
#include "../../Framework/Loaders/BasicFetchingItemsSorter.h"
#include "../../Framework/Loaders/BasicFetchingStrategy.h"
#include "../../Framework/Messages/ICallable.h"
#include "../../Framework/Messages/IMessage.h"
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Messages/MessageBroker.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/FloatTextureSceneLayer.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Toolbox/GeometryToolbox.h"
#include "../../Framework/Toolbox/SlicesSorter.h"
#include "../../Framework/Volumes/ImageBuffer3D.h"
#include "../../Framework/Volumes/VolumeImageGeometry.h"

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
#include <Core/SystemToolbox.h>
#include <Core/Toolbox.h>

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <list>
#include <stdio.h>



namespace OrthancStone
{
  typedef std::map<std::string, std::string>  HttpHeaders;

  class OrthancRestApiCommand : public OracleCommandWithPayload
  {
  public:
    class SuccessMessage : public OriginMessage<OrthancRestApiCommand>
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
    class SuccessMessage : public OriginMessage<GetOrthancImageCommand>
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
    std::string           uri_;
    HttpHeaders           headers_;
    unsigned int          timeout_;
    bool                  hasExpectedFormat_;
    Orthanc::PixelFormat  expectedFormat_;

  public:
    GetOrthancImageCommand() :
      uri_("/"),
      timeout_(10),
      hasExpectedFormat_(false)
    {
    }

    virtual Type GetType() const
    {
      return Type_GetOrthancImage;
    }

    void SetExpectedPixelFormat(Orthanc::PixelFormat format)
    {
      hasExpectedFormat_ = true;
      expectedFormat_ = format;
    }

    void SetUri(const std::string& uri)
    {
      uri_ = uri;
    }

    void SetInstanceUri(const std::string& instance,
                        Orthanc::PixelFormat pixelFormat)
    {
      uri_ = "/instances/" + instance;
          
      switch (pixelFormat)
      {
        case Orthanc::PixelFormat_RGB24:
          uri_ += "/preview";
          break;
      
        case Orthanc::PixelFormat_Grayscale16:
          uri_ += "/image-uint16";
          break;
      
        case Orthanc::PixelFormat_SignedGrayscale16:
          uri_ += "/image-int16";
          break;
      
        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
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
                           const IObserver& receiver,
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

      if (hasExpectedFormat_)
      {
        if (expectedFormat_ == Orthanc::PixelFormat_SignedGrayscale16 &&
            image->GetFormat() == Orthanc::PixelFormat_Grayscale16)
        {
          image->SetFormat(Orthanc::PixelFormat_SignedGrayscale16);
        }

        if (expectedFormat_ != image->GetFormat())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
        }
      }

      SuccessMessage message(*this, image.release(), contentType);
      emitter.EmitMessage(receiver, message);
    }
  };



  class GetOrthancWebViewerJpegCommand : public OracleCommandWithPayload
  {
  public:
    class SuccessMessage : public OriginMessage<GetOrthancWebViewerJpegCommand>
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

    void SetExpectedPixelFormat(Orthanc::PixelFormat format)
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

    Orthanc::PixelFormat GetExpectedPixelFormat() const
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
                           const IObserver& receiver,
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
    
      if (!LinearAlgebra::IsCloseToZero(scaling))
      {
        float offset = static_cast<float>(stretchLow) / scaling;
        Orthanc::ImageProcessing::ShiftScale(*image, offset, scaling, true);
      }
    
      SuccessMessage message(*this, image.release());
      emitter.EmitMessage(receiver, message);
    }
  };





  class DicomInstanceParameters :
    public Orthanc::IDynamicObject  /* to be used as a payload of SlicesSorter */
  {
  private:
    struct Data   // Struct to ease the copy constructor
    {
      std::string                       orthancInstanceId_;
      std::string                       studyInstanceUid_;
      std::string                       seriesInstanceUid_;
      std::string                       sopInstanceUid_;
      Orthanc::DicomImageInformation    imageInformation_;
      SopClassUid         sopClassUid_;
      double                            thickness_;
      double                            pixelSpacingX_;
      double                            pixelSpacingY_;
      CoordinateSystem3D  geometry_;
      Vector              frameOffsets_;
      bool                              isColor_;
      bool                              hasRescale_;
      double                            rescaleIntercept_;
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

        if (!LinearAlgebra::ParseVector(frameOffsets_, dicom, Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR) ||
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

      Data(const Orthanc::DicomMap& dicom) :
        imageInformation_(dicom)
      {
        if (imageInformation_.GetNumberOfFrames() <= 0)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }

        if (!dicom.CopyToString(studyInstanceUid_, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) ||
            !dicom.CopyToString(seriesInstanceUid_, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false) ||
            !dicom.CopyToString(sopInstanceUid_, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
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
          sopClassUid_ = StringToSopClassUid(s);
        }

        if (!dicom.ParseDouble(thickness_, Orthanc::DICOM_TAG_SLICE_THICKNESS))
        {
          thickness_ = 100.0 * std::numeric_limits<double>::epsilon();
        }

        GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, dicom);

        std::string position, orientation;
        if (dicom.CopyToString(position, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT, false) &&
            dicom.CopyToString(orientation, Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT, false))
        {
          geometry_ = CoordinateSystem3D(position, orientation);
        }

        if (sopClassUid_ == SopClassUid_RTDose)
        {
          ComputeDoseOffsets(dicom);
        }

        isColor_ = (imageInformation_.GetPhotometricInterpretation() != Orthanc::PhotometricInterpretation_Monochrome1 &&
                    imageInformation_.GetPhotometricInterpretation() != Orthanc::PhotometricInterpretation_Monochrome2);

        double doseGridScaling;

        if (dicom.ParseDouble(rescaleIntercept_, Orthanc::DICOM_TAG_RESCALE_INTERCEPT) &&
            dicom.ParseDouble(rescaleSlope_, Orthanc::DICOM_TAG_RESCALE_SLOPE))
        {
          hasRescale_ = true;
        }
        else if (dicom.ParseDouble(doseGridScaling, Orthanc::DICOM_TAG_DOSE_GRID_SCALING))
        {
          hasRescale_ = true;
          rescaleIntercept_ = 0;
          rescaleSlope_ = doseGridScaling;
        }
        else
        {
          hasRescale_ = false;
        }

        Vector c, w;
        if (LinearAlgebra::ParseVector(c, dicom, Orthanc::DICOM_TAG_WINDOW_CENTER) &&
            LinearAlgebra::ParseVector(w, dicom, Orthanc::DICOM_TAG_WINDOW_WIDTH) &&
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

        if (sopClassUid_ == SopClassUid_RTDose)
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

      CoordinateSystem3D  GetFrameGeometry(unsigned int frame) const
      {
        if (frame == 0)
        {
          return geometry_;
        }
        else if (frame >= imageInformation_.GetNumberOfFrames())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }
        else if (sopClassUid_ == SopClassUid_RTDose)
        {
          if (frame >= frameOffsets_.size())
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }

          return CoordinateSystem3D(
            geometry_.GetOrigin() + frameOffsets_[frame] * geometry_.GetNormal(),
            geometry_.GetAxisX(),
            geometry_.GetAxisY());
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }
      }

      // TODO - Is this necessary?
      bool FrameContainsPlane(unsigned int frame,
                              const CoordinateSystem3D& plane) const
      {
        if (frame >= imageInformation_.GetNumberOfFrames())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        CoordinateSystem3D tmp = geometry_;

        if (frame != 0)
        {
          tmp = GetFrameGeometry(frame);
        }

        double distance;

        return (CoordinateSystem3D::GetDistance(distance, tmp, plane) &&
                distance <= thickness_ / 2.0);
      }

      
      void ApplyRescale(Orthanc::ImageAccessor& image,
                        bool useDouble) const
      {
        if (image.GetFormat() != Orthanc::PixelFormat_Float32)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
        }
    
        if (hasRescale_)
        {
          const unsigned int width = image.GetWidth();
          const unsigned int height = image.GetHeight();
        
          for (unsigned int y = 0; y < height; y++)
          {
            float* p = reinterpret_cast<float*>(image.GetRow(y));

            if (useDouble)
            {
              // Slower, accurate implementation using double
              for (unsigned int x = 0; x < width; x++, p++)
              {
                double value = static_cast<double>(*p);
                *p = static_cast<float>(value * rescaleSlope_ + rescaleIntercept_);
              }
            }
            else
            {
              // Fast, approximate implementation using float
              for (unsigned int x = 0; x < width; x++, p++)
              {
                *p = (*p) * static_cast<float>(rescaleSlope_) + static_cast<float>(rescaleIntercept_);
              }
            }
          }
        }
      }
    };

    
    Data  data_;


  public:
    DicomInstanceParameters(const DicomInstanceParameters& other) :
    data_(other.data_)
    {
    }

    DicomInstanceParameters(const Orthanc::DicomMap& dicom) :
      data_(dicom)
    {
    }

    void SetOrthancInstanceIdentifier(const std::string& id)
    {
      data_.orthancInstanceId_ = id;
    }

    const std::string& GetOrthancInstanceIdentifier() const
    {
      return data_.orthancInstanceId_;
    }

    const Orthanc::DicomImageInformation& GetImageInformation() const
    {
      return data_.imageInformation_;
    }

    const std::string& GetStudyInstanceUid() const
    {
      return data_.studyInstanceUid_;
    }

    const std::string& GetSeriesInstanceUid() const
    {
      return data_.seriesInstanceUid_;
    }

    const std::string& GetSopInstanceUid() const
    {
      return data_.sopInstanceUid_;
    }

    SopClassUid GetSopClassUid() const
    {
      return data_.sopClassUid_;
    }

    double GetThickness() const
    {
      return data_.thickness_;
    }

    double GetPixelSpacingX() const
    {
      return data_.pixelSpacingX_;
    }

    double GetPixelSpacingY() const
    {
      return data_.pixelSpacingY_;
    }

    const CoordinateSystem3D&  GetGeometry() const
    {
      return data_.geometry_;
    }

    CoordinateSystem3D  GetFrameGeometry(unsigned int frame) const
    {
      return data_.GetFrameGeometry(frame);
    }

    // TODO - Is this necessary?
    bool FrameContainsPlane(unsigned int frame,
                            const CoordinateSystem3D& plane) const
    {
      return data_.FrameContainsPlane(frame, plane);
    }

    bool IsColor() const
    {
      return data_.isColor_;
    }

    bool HasRescale() const
    {
      return data_.hasRescale_;
    }

    double GetRescaleIntercept() const
    {
      if (data_.hasRescale_)
      {
        return data_.rescaleIntercept_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    double GetRescaleSlope() const
    {
      if (data_.hasRescale_)
      {
        return data_.rescaleSlope_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    bool HasDefaultWindowing() const
    {
      return data_.hasDefaultWindowing_;
    }

    float GetDefaultWindowingCenter() const
    {
      if (data_.hasDefaultWindowing_)
      {
        return data_.defaultWindowingCenter_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    float GetDefaultWindowingWidth() const
    {
      if (data_.hasDefaultWindowing_)
      {
        return data_.defaultWindowingWidth_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    Orthanc::PixelFormat GetExpectedPixelFormat() const
    {
      return data_.expectedPixelFormat_;
    }


    TextureBaseSceneLayer* CreateTexture(const Orthanc::ImageAccessor& source) const
    {
      assert(sizeof(float) == 4);

      Orthanc::PixelFormat sourceFormat = source.GetFormat();

      if (sourceFormat != GetExpectedPixelFormat())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }

      if (sourceFormat == Orthanc::PixelFormat_RGB24)
      {
        // This is the case of a color image. No conversion has to be done.
        return new ColorTextureSceneLayer(source);
      }
      else
      {
        if (sourceFormat != Orthanc::PixelFormat_Grayscale16 &&
            sourceFormat != Orthanc::PixelFormat_Grayscale32 &&
            sourceFormat != Orthanc::PixelFormat_SignedGrayscale16)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }

        std::auto_ptr<FloatTextureSceneLayer> texture;
        
        {
          // This is the case of a grayscale frame. Convert it to Float32.
          std::auto_ptr<Orthanc::Image> converted(new Orthanc::Image(Orthanc::PixelFormat_Float32, 
                                                                     source.GetWidth(), 
                                                                     source.GetHeight(),
                                                                     false));
          Orthanc::ImageProcessing::Convert(*converted, source);

          // Correct rescale slope/intercept if need be
          data_.ApplyRescale(*converted, (sourceFormat == Orthanc::PixelFormat_Grayscale32));

          texture.reset(new FloatTextureSceneLayer(*converted));
        }

        if (data_.hasDefaultWindowing_)
        {
          texture->SetCustomWindowing(data_.defaultWindowingCenter_,
                                      data_.defaultWindowingWidth_);
        }
        
        return texture.release();
      }
    }
  };


  class DicomVolumeImage : public boost::noncopyable
  {
  private:
    std::auto_ptr<ImageBuffer3D>  image_;
    std::auto_ptr<VolumeImageGeometry>  geometry_;
    std::vector<DicomInstanceParameters*>       slices_;
    uint64_t                                    revision_;
    std::vector<uint64_t>                       slicesRevision_;
    std::vector<unsigned int>                   slicesQuality_;

    void CheckSlice(size_t index,
                    const DicomInstanceParameters& reference) const
    {
      const DicomInstanceParameters& slice = *slices_[index];
      
      if (!GeometryToolbox::IsParallel(
            reference.GetGeometry().GetNormal(),
            slice.GetGeometry().GetNormal()))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadGeometry,
                                        "A slice in the volume image is not parallel to the others");
      }

      if (reference.GetExpectedPixelFormat() != slice.GetExpectedPixelFormat())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat,
                                        "The pixel format changes across the slices of the volume image");
      }

      if (reference.GetImageInformation().GetWidth() != slice.GetImageInformation().GetWidth() ||
          reference.GetImageInformation().GetHeight() != slice.GetImageInformation().GetHeight())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize,
                                        "The width/height of slices are not constant in the volume image");
      }

      if (!LinearAlgebra::IsNear(reference.GetPixelSpacingX(), slice.GetPixelSpacingX()) ||
          !LinearAlgebra::IsNear(reference.GetPixelSpacingY(), slice.GetPixelSpacingY()))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadGeometry,
                                        "The pixel spacing of the slices change across the volume image");
      }
    }

    
    void CheckVolume() const
    {
      for (size_t i = 0; i < slices_.size(); i++)
      {
        assert(slices_[i] != NULL);
        if (slices_[i]->GetImageInformation().GetNumberOfFrames() != 1)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadGeometry,
                                          "This class does not support multi-frame images");
        }
      }

      if (slices_.size() != 0)
      {
        const DicomInstanceParameters& reference = *slices_[0];

        for (size_t i = 1; i < slices_.size(); i++)
        {
          CheckSlice(i, reference);
        }
      }
    }


    void Clear()
    {
      image_.reset();
      geometry_.reset();
      
      for (size_t i = 0; i < slices_.size(); i++)
      {
        assert(slices_[i] != NULL);
        delete slices_[i];
      }

      slices_.clear();
      slicesRevision_.clear();
      slicesQuality_.clear();
    }


    void CheckSliceIndex(size_t index) const
    {
      assert(slices_.size() == image_->GetDepth() &&
             slices_.size() == slicesRevision_.size());

      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else if (index >= slices_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    
  public:
    DicomVolumeImage()
    {
    }

    ~DicomVolumeImage()
    {
      Clear();
    }

    // WARNING: The payload of "slices" must be of class "DicomInstanceParameters"
    void SetGeometry(SlicesSorter& slices)
    {
      Clear();
      
      if (!slices.Sort())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                        "Cannot sort the 3D slices of a DICOM series");          
      }

      geometry_.reset(new VolumeImageGeometry);

      if (slices.GetSlicesCount() == 0)
      {
        // Empty volume
        image_.reset(new ImageBuffer3D(Orthanc::PixelFormat_Grayscale8, 0, 0, 0,
                                       false /* don't compute range */));
      }
      else
      {
        slices_.reserve(slices.GetSlicesCount());
        slicesRevision_.resize(slices.GetSlicesCount(), 0);
        slicesQuality_.resize(slices.GetSlicesCount(), 0);

        for (size_t i = 0; i < slices.GetSlicesCount(); i++)
        {
          const DicomInstanceParameters& slice =
            dynamic_cast<const DicomInstanceParameters&>(slices.GetSlicePayload(i));
          slices_.push_back(new DicomInstanceParameters(slice));
        }

        CheckVolume();

        const double spacingZ = slices.ComputeSpacingBetweenSlices();
        LOG(INFO) << "Computed spacing between slices: " << spacingZ << "mm";
      
        const DicomInstanceParameters& parameters = *slices_[0];

        image_.reset(new ImageBuffer3D(parameters.GetExpectedPixelFormat(),
                                       parameters.GetImageInformation().GetWidth(),
                                       parameters.GetImageInformation().GetHeight(),
                                       slices.GetSlicesCount(), false /* don't compute range */));      

        geometry_->SetSize(image_->GetWidth(), image_->GetHeight(), image_->GetDepth());
        geometry_->SetAxialGeometry(slices.GetSliceGeometry(0));
        geometry_->SetVoxelDimensions(parameters.GetPixelSpacingX(),
                                      parameters.GetPixelSpacingY(), spacingZ);
      }
      
      image_->Clear();

      revision_++;
    }

    uint64_t GetRevision() const
    {
      return revision_;
    }

    bool HasGeometry() const
    {
      return (image_.get() != NULL &&
              geometry_.get() != NULL);
    }

    const ImageBuffer3D& GetImage() const
    {
      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return *image_;
      }
    }

    const VolumeImageGeometry& GetGeometry() const
    {
      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return *geometry_;
      }
    }

    size_t GetSlicesCount() const
    {
      if (!HasGeometry())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return slices_.size();
      }
    }

    const DicomInstanceParameters& GetSliceParameters(size_t index) const
    {
      CheckSliceIndex(index);
      return *slices_[index];
    }

    uint64_t GetSliceRevision(size_t index) const
    {
      CheckSliceIndex(index);
      return slicesRevision_[index];
    }

    void SetSliceContent(size_t index,
                         const Orthanc::ImageAccessor& image,
                         unsigned int quality)
    {
      CheckSliceIndex(index);

      // If a better image quality is already available, don't update the content
      if (quality >= slicesQuality_[index])
      {
        {
          ImageBuffer3D::SliceWriter writer
            (*image_, VolumeProjection_Axial, index);
          Orthanc::ImageProcessing::Copy(writer.GetAccessor(), image);
        }
        
        revision_ ++;
        slicesRevision_[index] += 1;
      }
    }
  };



  class IDicomVolumeSource : public boost::noncopyable
  {
  public:
    virtual ~IDicomVolumeSource()
    {
    }

    virtual const DicomVolumeImage& GetVolume() const = 0;

    virtual void NotifyAxialSliceAccessed(unsigned int sliceIndex) = 0;
  };
  
  

  class VolumeSeriesOrthancLoader :
    public IObserver,
    public IDicomVolumeSource
  {
  private:
    static const unsigned int LOW_QUALITY = 0;
    static const unsigned int MIDDLE_QUALITY = 1;
    static const unsigned int BEST_QUALITY = 2;
    
    
    static unsigned int GetSliceIndexPayload(const OracleCommandWithPayload& command)
    {
      return dynamic_cast< const Orthanc::SingleValueObject<unsigned int>& >(command.GetPayload()).GetValue();
    }


    void ScheduleNextSliceDownload()
    {
      assert(strategy_.get() != NULL);
      
      unsigned int sliceIndex, quality;
      
      if (strategy_->GetNext(sliceIndex, quality))
      {
        assert(quality <= BEST_QUALITY);

        const DicomInstanceParameters& slice = volume_.GetSliceParameters(sliceIndex);
          
        const std::string& instance = slice.GetOrthancInstanceIdentifier();
        if (instance.empty())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        std::auto_ptr<OracleCommandWithPayload> command;
        
        if (quality == BEST_QUALITY)
        {
          std::auto_ptr<GetOrthancImageCommand> tmp(new GetOrthancImageCommand);
          tmp->SetHttpHeader("Accept-Encoding", "gzip");
          tmp->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
          tmp->SetInstanceUri(instance, slice.GetExpectedPixelFormat());          
          tmp->SetExpectedPixelFormat(slice.GetExpectedPixelFormat());
          command.reset(tmp.release());
        }
        else
        {
          std::auto_ptr<GetOrthancWebViewerJpegCommand> tmp(new GetOrthancWebViewerJpegCommand);
          tmp->SetHttpHeader("Accept-Encoding", "gzip");
          tmp->SetInstance(instance);
          tmp->SetQuality((quality == 0 ? 50 : 90));
          tmp->SetExpectedPixelFormat(slice.GetExpectedPixelFormat());
          command.reset(tmp.release());
        }

        command->SetPayload(new Orthanc::SingleValueObject<unsigned int>(sliceIndex));
        oracle_.Schedule(*this, command.release());
      }
    }


    void LoadGeometry(const OrthancRestApiCommand::SuccessMessage& message)
    {
      Json::Value body;
      message.ParseJsonBody(body);
      
      if (body.type() != Json::objectValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      {
        Json::Value::Members instances = body.getMemberNames();

        SlicesSorter slices;
        
        for (size_t i = 0; i < instances.size(); i++)
        {
          Orthanc::DicomMap dicom;
          dicom.FromDicomAsJson(body[instances[i]]);

          std::auto_ptr<DicomInstanceParameters> instance(new DicomInstanceParameters(dicom));
          instance->SetOrthancInstanceIdentifier(instances[i]);

          CoordinateSystem3D geometry = instance->GetGeometry();
          slices.AddSlice(geometry, instance.release());
        }

        volume_.SetGeometry(slices);
      }

      if (volume_.GetSlicesCount() != 0)
      {
        strategy_.reset(new BasicFetchingStrategy(
                          new BasicFetchingItemsSorter(volume_.GetSlicesCount()), BEST_QUALITY));

        for (unsigned int i = 0; i < 4; i++)   // Schedule up to 4 simultaneous downloads (TODO - parameter)
        {
          ScheduleNextSliceDownload();
        }
      }
    }


    void LoadBestQualitySliceContent(const GetOrthancImageCommand::SuccessMessage& message)
    {      
      volume_.SetSliceContent(GetSliceIndexPayload(message.GetOrigin()),
                              message.GetImage(), BEST_QUALITY);

      ScheduleNextSliceDownload();
    }


    void LoadJpegSliceContent(const GetOrthancWebViewerJpegCommand::SuccessMessage& message)
    {
      unsigned int quality;
      
      switch (message.GetOrigin().GetQuality())
      {
        case 50:
          quality = LOW_QUALITY;
          break;

        case 90:
          quality = MIDDLE_QUALITY;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      
      volume_.SetSliceContent(GetSliceIndexPayload(message.GetOrigin()), message.GetImage(), quality);

      ScheduleNextSliceDownload();
    }


    IOracle&          oracle_;
    bool              active_;
    DicomVolumeImage  volume_;
    
    std::auto_ptr<IFetchingStrategy>   strategy_;

  public:
    VolumeSeriesOrthancLoader(IOracle& oracle,
                              IObservable& oracleObservable) :
      IObserver(oracleObservable.GetBroker()),
      oracle_(oracle),
      active_(false)
    {
      oracleObservable.RegisterObserverCallback(
        new Callable<VolumeSeriesOrthancLoader, OrthancRestApiCommand::SuccessMessage>
        (*this, &VolumeSeriesOrthancLoader::LoadGeometry));

      oracleObservable.RegisterObserverCallback(
        new Callable<VolumeSeriesOrthancLoader, GetOrthancImageCommand::SuccessMessage>
        (*this, &VolumeSeriesOrthancLoader::LoadBestQualitySliceContent));

      oracleObservable.RegisterObserverCallback(
        new Callable<VolumeSeriesOrthancLoader, GetOrthancWebViewerJpegCommand::SuccessMessage>
        (*this, &VolumeSeriesOrthancLoader::LoadJpegSliceContent));
    }

    void LoadSeries(const std::string& seriesId)
    {
      if (active_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      active_ = true;

      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetUri("/series/" + seriesId + "/instances-tags");

      oracle_.Schedule(*this, command.release());
    }
    

    virtual const DicomVolumeImage& GetVolume() const
    {
      return volume_;
    }

    
    virtual void NotifyAxialSliceAccessed(unsigned int sliceIndex)
    {
      if (strategy_.get() == NULL)
      {
        // Should have called GetVolume().HasGeometry() before
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        strategy_->SetCurrent(sliceIndex);
      }
    }
  };



#if 0
  void LoadInstance(const std::string& instanceId)
  {
    if (active_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    active_ = true;

    // Tag "3004-000c" is "Grid Frame Offset Vector", which is
    // mandatory to read RT DOSE, but is too long to be returned by default

    // TODO => Should be part of a second call if needed

    std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
    command->SetUri("/instances/" + instanceId + "/tags?ignore-length=3004-000c");
    command->SetPayload(new LoadInstanceGeometryHandler(*this));

    oracle_.Schedule(*this, command.release());
  }
#endif


  /*  class VolumeSlicerBase : public IVolumeSlicer
      {
      private:
      Scene2D&            scene_;
      int                               layerDepth_;
      bool                              first_;
      CoordinateSystem3D  lastPlane_;

      protected:
      bool HasViewportPlaneChanged(const CoordinateSystem3D& plane) const
      {
      if (first_ ||
      !LinearAlgebra::IsCloseToZero(
      boost::numeric::ublas::norm_2(lastPlane_.GetNormal() - plane.GetNormal())))
      {
      // This is the first rendering, or the plane has not the same orientation
      return false;
      }
      else
      {
      double offset1 = lastPlane_.ProjectAlongNormal(plane.GetOrigin());
      double offset2 = lastPlane_.ProjectAlongNormal(lastPlane_.GetOrigin());
      return LinearAlgebra::IsCloseToZero(offset2 - offset1);
      }
      }

      void SetLastViewportPlane(const CoordinateSystem3D& plane)
      {
      first_ = false;
      lastPlane_ = plane;
      }

      void SetLayer(ISceneLayer* layer)
      {
      scene_.SetLayer(layerDepth_, layer);
      }

      void DeleteLayer()
      {
      scene_.DeleteLayer(layerDepth_);
      }
    
      public:
      VolumeSlicerBase(Scene2D& scene,
      int layerDepth) :
      scene_(scene),
      layerDepth_(layerDepth),
      first_(true)
      {
      }
      };*/
  


  class IVolumeSlicer : public boost::noncopyable
  {
  public:
    virtual ~IVolumeSlicer()
    {
    }

    virtual void SetViewportPlane(const CoordinateSystem3D& plane) = 0;
  };


  class DicomVolumeMPRSlicer : public IVolumeSlicer
  {
  private:
    bool                            linearInterpolation_;
    Scene2D&          scene_;
    int                             layerDepth_;
    IDicomVolumeSource&             source_;
    bool                            first_;
    VolumeProjection  lastProjection_;
    unsigned int                    lastSliceIndex_;
    uint64_t                        lastSliceRevision_;

  public:
    DicomVolumeMPRSlicer(Scene2D& scene,
                         int layerDepth,
                         IDicomVolumeSource& source) :
      linearInterpolation_(false),
      scene_(scene),
      layerDepth_(layerDepth),
      source_(source),
      first_(true)
    {
    }

    void SetLinearInterpolation(bool enabled)
    {
      linearInterpolation_ = enabled;
    }

    bool IsLinearInterpolation() const
    {
      return linearInterpolation_;
    }
    
    virtual void SetViewportPlane(const CoordinateSystem3D& plane)
    {
      if (!source_.GetVolume().HasGeometry() ||
          source_.GetVolume().GetSlicesCount() == 0)
      {
        scene_.DeleteLayer(layerDepth_);
        return;
      }

      const VolumeImageGeometry& geometry = source_.GetVolume().GetGeometry();

      VolumeProjection projection;
      unsigned int sliceIndex;
      if (!geometry.DetectSlice(projection, sliceIndex, plane))
      {
        // The cutting plane is neither axial, nor coronal, nor
        // sagittal. Could use "VolumeReslicer" here.
        scene_.DeleteLayer(layerDepth_);
        return;
      }

      uint64_t sliceRevision;
      if (projection == VolumeProjection_Axial)
      {
        sliceRevision = source_.GetVolume().GetSliceRevision(sliceIndex);

        if (first_ ||
            lastSliceIndex_ != sliceIndex)
        {
          // Reorder the prefetching queue
          source_.NotifyAxialSliceAccessed(sliceIndex);
        }
      }
      else
      {
        // For coronal and sagittal projections, we take the global
        // revision of the volume
        sliceRevision = source_.GetVolume().GetRevision();
      }

      if (first_ ||
          lastProjection_ != projection ||
          lastSliceIndex_ != sliceIndex ||
          lastSliceRevision_ != sliceRevision)
      {
        // Either the viewport plane, or the content of the slice have not
        // changed since the last time the layer was set: Update is needed

        first_ = false;
        lastProjection_ = projection;
        lastSliceIndex_ = sliceIndex;
        lastSliceRevision_ = sliceRevision;

        std::auto_ptr<TextureBaseSceneLayer> texture;
        
        {
          const DicomInstanceParameters& parameters = source_.GetVolume().GetSliceParameters
            (projection == VolumeProjection_Axial ? sliceIndex : 0);

          ImageBuffer3D::SliceReader reader(source_.GetVolume().GetImage(), projection, sliceIndex);
          texture.reset(parameters.CreateTexture(reader.GetAccessor()));
        }

        const CoordinateSystem3D& system = geometry.GetProjectionGeometry(projection);

        double x0, y0, x1, y1;
        system.ProjectPoint(x0, y0, system.GetOrigin());
        system.ProjectPoint(x0, y0, system.GetOrigin() + system.GetAxisX());
        texture->SetOrigin(x0, y0);

        double dx = x1 - x0;
        double dy = y1 - y0;
        if (!LinearAlgebra::IsCloseToZero(dx) ||
            !LinearAlgebra::IsCloseToZero(dy))
        {
          texture->SetAngle(atan2(dy, dx));
        }
        
        Vector tmp;
        geometry.GetVoxelDimensions(projection);
        texture->SetPixelSpacing(tmp[0], tmp[1]);

        texture->SetLinearInterpolation(linearInterpolation_);
    
        scene_.SetLayer(layerDepth_, texture.release());    
      }
    }
  };
  
  



  class ThreadedOracle : public IOracle
  {
  private:
    class Item : public Orthanc::IDynamicObject
    {
    private:
      const IObserver&  receiver_;
      std::auto_ptr<IOracleCommand>   command_;

    public:
      Item(const IObserver& receiver,
           IOracleCommand* command) :
        receiver_(receiver),
        command_(command)
      {
        if (command == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      const IObserver& GetReceiver() const
      {
        return receiver_;
      }

      IOracleCommand& GetCommand()
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


    class SleepingCommands : public boost::noncopyable
    {
    private:
      class Item
      {
      private:
        const IObserver&     receiver_;
        std::auto_ptr<SleepOracleCommand>  command_;
        boost::posix_time::ptime           expiration_;

      public:
        Item(const IObserver& receiver,
             SleepOracleCommand* command) :
          receiver_(receiver),
          command_(command)
        {
          if (command == NULL)
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
          }

          expiration_ = (boost::posix_time::second_clock::local_time() + 
                         boost::posix_time::milliseconds(command_->GetDelay()));
        }

        const boost::posix_time::ptime& GetExpirationTime() const
        {
          return expiration_;
        }

        void Awake(IMessageEmitter& emitter)
        {
          assert(command_.get() != NULL);

          SleepOracleCommand::TimeoutMessage message(*command_);
          emitter.EmitMessage(receiver_, message);
        }
      };

      typedef std::list<Item*>  Content;

      boost::mutex  mutex_;
      Content       content_;

    public:
      ~SleepingCommands()
      {
        for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
        {
          if (*it != NULL)
          {
            delete *it;
          }
        }
      }

      void Add(const IObserver& receiver,
               SleepOracleCommand* command)   // Takes ownership
      {
        boost::mutex::scoped_lock lock(mutex_);

        content_.push_back(new Item(receiver, command));
      }

      void AwakeExpired(IMessageEmitter& emitter)
      {
        boost::mutex::scoped_lock lock(mutex_);

        const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

        Content  stillSleeping;
        
        for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
        {
          if (*it != NULL &&
              (*it)->GetExpirationTime() <= now)
          {
            (*it)->Awake(emitter);
            delete *it;
            *it = NULL;
          }
          else
          {
            stillSleeping.push_back(*it);
          }
        }

        // Compact the still-sleeping commands
        content_ = stillSleeping;
      }
    };


    IMessageEmitter&               emitter_;
    Orthanc::WebServiceParameters  orthanc_;
    Orthanc::SharedMessageQueue    queue_;
    State                          state_;
    boost::mutex                   mutex_;
    std::vector<boost::thread*>    workers_;
    SleepingCommands               sleepingCommands_;
    boost::thread                  sleepingWorker_;
    unsigned int                   sleepingTimeResolution_;

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


    void Execute(const IObserver& receiver,
                 SleepOracleCommand& command)
    {
      std::auto_ptr<SleepOracleCommand> copy(new SleepOracleCommand(command.GetDelay()));

      if (command.HasPayload())
      {
        copy->SetPayload(command.ReleasePayload());
      }

      sleepingCommands_.Add(receiver, copy.release());
    }


    void Execute(const IObserver& receiver,
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


    void Execute(const IObserver& receiver,
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


    void Execute(const IObserver& receiver,
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
        Item& item = dynamic_cast<Item&>(*object);

        try
        {
          switch (item.GetCommand().GetType())
          {
            case IOracleCommand::Type_Sleep:
              Execute(item.GetReceiver(), 
                      dynamic_cast<SleepOracleCommand&>(item.GetCommand()));
              break;

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


    static void Worker(ThreadedOracle* that)
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


    static void SleepingWorker(ThreadedOracle* that)
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

        that->sleepingCommands_.AwakeExpired(that->emitter_);

        boost::this_thread::sleep(boost::posix_time::milliseconds(that->sleepingTimeResolution_));
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

      if (sleepingWorker_.joinable())
      {
        sleepingWorker_.join();
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
    ThreadedOracle(IMessageEmitter& emitter) :
    emitter_(emitter),
    state_(State_Setup),
    workers_(4),
    sleepingTimeResolution_(50)  // By default, time resolution of 50ms
    {
    }

    virtual ~ThreadedOracle()
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

    void SetSleepingTimeResolution(unsigned int milliseconds)
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (milliseconds <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else if (state_ != State_Setup)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        sleepingTimeResolution_ = milliseconds;
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

        sleepingWorker_ = boost::thread(SleepingWorker, this);
      }      
    }

    void Stop()
    {
      StopInternal();
    }

    virtual void Schedule(const IObserver& receiver,
                          IOracleCommand* command)
    {
      queue_.Enqueue(new Item(receiver, command));
    }
  };


  class NativeApplicationContext : public IMessageEmitter
  {
  private:
    boost::shared_mutex            mutex_;
    MessageBroker    broker_;
    IObservable      oracleObservable_;

  public:
    NativeApplicationContext() :
      oracleObservable_(broker_)
    {
    }


    virtual void EmitMessage(const IObserver& observer,
                             const IMessage& message)
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

      MessageBroker& GetBroker() 
      {
        return that_.broker_;
      }

      IObservable& GetOracleObservable()
      {
        return that_.oracleObservable_;
      }
    };
  };
}



class Toto : public OrthancStone::IObserver
{
private:
  void Handle(const OrthancStone::SleepOracleCommand::TimeoutMessage& message)
  {
    printf("TIMEOUT! %d\n", dynamic_cast<const Orthanc::SingleValueObject<unsigned int>& >(message.GetOrigin().GetPayload()).GetValue());
  }

  void Handle(const OrthancStone::OrthancRestApiCommand::SuccessMessage& message)
  {
    Json::Value v;
    message.ParseJsonBody(v);

    printf("ICI [%s]\n", v.toStyledString().c_str());
  }

  void Handle(const OrthancStone::GetOrthancImageCommand::SuccessMessage& message)
  {
    printf("IMAGE %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage& message)
  {
    printf("WebViewer %dx%d\n", message.GetImage().GetWidth(), message.GetImage().GetHeight());
  }

  void Handle(const OrthancStone::OracleCommandExceptionMessage& message)
  {
    printf("EXCEPTION: [%s] on command type %d\n", message.GetException().What(), message.GetCommand().GetType());

    switch (message.GetCommand().GetType())
    {
      case OrthancStone::IOracleCommand::Type_GetOrthancWebViewerJpeg:
        printf("URI: [%s]\n", dynamic_cast<const OrthancStone::GetOrthancWebViewerJpegCommand&>
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
       <Toto, OrthancStone::SleepOracleCommand::TimeoutMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OrthancRestApiCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancImageCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage>(*this, &Toto::Handle));

    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, OrthancStone::OracleCommandExceptionMessage>(*this, &Toto::Handle));
  }
};


void Run(OrthancStone::NativeApplicationContext& context,
         OrthancStone::IOracle& oracle)
{
  std::auto_ptr<Toto> toto;
  std::auto_ptr<OrthancStone::VolumeSeriesOrthancLoader> loader1, loader2;

  {
    OrthancStone::NativeApplicationContext::WriterLock lock(context);
    toto.reset(new Toto(lock.GetOracleObservable()));
    loader1.reset(new OrthancStone::VolumeSeriesOrthancLoader(oracle, lock.GetOracleObservable()));
    loader2.reset(new OrthancStone::VolumeSeriesOrthancLoader(oracle, lock.GetOracleObservable()));
  }

  if (0)
  {
    Json::Value v = Json::objectValue;
    v["Level"] = "Series";
    v["Query"] = Json::objectValue;

    std::auto_ptr<OrthancStone::OrthancRestApiCommand>  command(new OrthancStone::OrthancRestApiCommand);
    command->SetMethod(Orthanc::HttpMethod_Post);
    command->SetUri("/tools/find");
    command->SetBody(v);

    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Jpeg)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/preview");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Png)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept-Encoding", "gzip");
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }
  
  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancImageCommand>  command(new OrthancStone::GetOrthancImageCommand);
    command->SetHttpHeader("Accept", std::string(Orthanc::EnumerationToString(Orthanc::MimeType_Pam)));
    command->SetUri("/instances/6687cc73-07cae193-52ff29c8-f646cb16-0753ed92/image-uint16");
    oracle.Schedule(*toto, command.release());
  }

  if (0)
  {
    std::auto_ptr<OrthancStone::GetOrthancWebViewerJpegCommand>  command(new OrthancStone::GetOrthancWebViewerJpegCommand);
    command->SetHttpHeader("Accept-Encoding", "gzip");
    command->SetInstance("e6c7c20b-c9f65d7e-0d76f2e2-830186f2-3e3c600e");
    command->SetQuality(90);
    oracle.Schedule(*toto, command.release());
  }


  if (0)
  {
    for (unsigned int i = 0; i < 10; i++)
    {
      std::auto_ptr<OrthancStone::SleepOracleCommand> command(new OrthancStone::SleepOracleCommand(i * 1000));
      command->SetPayload(new Orthanc::SingleValueObject<unsigned int>(42 * i));
      oracle.Schedule(*toto, command.release());
    }
  }

  // 2017-11-17-Anonymized
  //loader1->LoadSeries("cb3ea4d1-d08f3856-ad7b6314-74d88d77-60b05618");  // CT
  //loader2->LoadInstance("41029085-71718346-811efac4-420e2c15-d39f99b6");  // RT-DOSE

  // Delphine
  //loader1->LoadSeries("5990e39c-51e5f201-fe87a54c-31a55943-e59ef80e");  // CT
  loader1->LoadSeries("67f1b334-02c16752-45026e40-a5b60b6b-030ecab5");  // Lung 1/10mm

  LOG(WARNING) << "...Waiting for Ctrl-C...";
  Orthanc::SystemToolbox::ServerBarrier();
  //boost::this_thread::sleep(boost::posix_time::seconds(1));
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
    OrthancStone::NativeApplicationContext context;

    OrthancStone::ThreadedOracle oracle(context);

    {
      Orthanc::WebServiceParameters p;
      //p.SetUrl("http://localhost:8043/");
      p.SetCredentials("orthanc", "orthanc");
      oracle.SetOrthancParameters(p);
    }

    oracle.Start();

    Run(context, oracle);

    oracle.Stop();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::StoneFinalize();

  return 0;
}
