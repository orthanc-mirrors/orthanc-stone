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


#include "SeriesThumbnailsLoader.h"

#include <Core/DicomFormat/DicomMap.h>
#include <Core/DicomFormat/DicomInstanceHasher.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/JpegWriter.h>
#include <Core/OrthancException.h>

#include <boost/algorithm/string/predicate.hpp>

static const unsigned int JPEG_QUALITY = 70;  // Only used for Orthanc source

namespace OrthancStone
{
  static SeriesThumbnailType ExtractSopClassUid(const std::string& sopClassUid)
  {
    if (sopClassUid == "1.2.840.10008.5.1.4.1.1.104.1")  // Encapsulated PDF Storage
    {
      return SeriesThumbnailType_Pdf;
    }
    else if (sopClassUid == "1.2.840.10008.5.1.4.1.1.77.1.1.1" ||  // Video Endoscopic Image Storage
             sopClassUid == "1.2.840.10008.5.1.4.1.1.77.1.2.1" ||  // Video Microscopic Image Storage
             sopClassUid == "1.2.840.10008.5.1.4.1.1.77.1.4.1")    // Video Photographic Image Storage
    {
      return SeriesThumbnailType_Video;
    }
    else
    {
      return SeriesThumbnailType_Unknown;
    }
  }


  SeriesThumbnailsLoader::Thumbnail::Thumbnail(const std::string& image,
                                               const std::string& mime) :
    type_(SeriesThumbnailType_Image),
    image_(image),
    mime_(mime)
  {
  }


  SeriesThumbnailsLoader::Thumbnail::Thumbnail(SeriesThumbnailType type) :
    type_(type)
  {
    if (type == SeriesThumbnailType_Image)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  void SeriesThumbnailsLoader::AcquireThumbnail(const DicomSource& source,
                                                const std::string& studyInstanceUid,
                                                const std::string& seriesInstanceUid,
                                                SeriesThumbnailsLoader::Thumbnail* thumbnail)
  {
    assert(thumbnail != NULL);
  
    std::auto_ptr<Thumbnail> protection(thumbnail);
  
    Thumbnails::iterator found = thumbnails_.find(seriesInstanceUid);
    if (found == thumbnails_.end())
    {
      thumbnails_[seriesInstanceUid] = protection.release();
    }
    else
    {
      assert(found->second != NULL);
      delete found->second;
      found->second = protection.release();
    }

    ThumbnailLoadedMessage message(*this, source, studyInstanceUid, seriesInstanceUid, *thumbnail);
    BroadcastMessage(message);
  }


  class SeriesThumbnailsLoader::Handler : public Orthanc::IDynamicObject
  {
  private:
    boost::shared_ptr<SeriesThumbnailsLoader>  loader_;
    DicomSource                                source_;
    std::string                                studyInstanceUid_;
    std::string                                seriesInstanceUid_;

  public:
    Handler(boost::shared_ptr<SeriesThumbnailsLoader> loader,
            const DicomSource& source,
            const std::string& studyInstanceUid,
            const std::string& seriesInstanceUid) :
      loader_(loader),
      source_(source),
      studyInstanceUid_(studyInstanceUid),
      seriesInstanceUid_(seriesInstanceUid)
    {
      if (!loader)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    boost::shared_ptr<SeriesThumbnailsLoader> GetLoader()
    {
      return loader_;
    }

    const DicomSource& GetSource() const
    {
      return source_;
    }

    const std::string& GetStudyInstanceUid() const
    {
      return studyInstanceUid_;
    }

    const std::string& GetSeriesInstanceUid() const
    {
      return seriesInstanceUid_;
    }

    virtual void HandleSuccess(const std::string& body,
                               const std::map<std::string, std::string>& headers) = 0;

    virtual void HandleError()
    {
      LOG(INFO) << "Cannot generate thumbnail for SeriesInstanceUID: " << seriesInstanceUid_;
    }
  };


  class SeriesThumbnailsLoader::DicomWebSopClassHandler : public SeriesThumbnailsLoader::Handler
  {
  private:
    static bool GetSopClassUid(std::string& sopClassUid,
                               const Json::Value& json)
    {
      Orthanc::DicomMap dicom;
      dicom.FromDicomWeb(json);

      return dicom.LookupStringValue(sopClassUid, Orthanc::DICOM_TAG_SOP_CLASS_UID, false);
    }
  
  public:
    DicomWebSopClassHandler(boost::shared_ptr<SeriesThumbnailsLoader> loader,
                            const DicomSource& source,
                            const std::string& studyInstanceUid,
                            const std::string& seriesInstanceUid) :
      Handler(loader, source, studyInstanceUid, seriesInstanceUid)
    {
    }

    virtual void HandleSuccess(const std::string& body,
                               const std::map<std::string, std::string>& headers)
    {
      Json::Reader reader;
      Json::Value value;

      if (!reader.parse(body, value) ||
          value.type() != Json::arrayValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }
      else
      {
        SeriesThumbnailType type = SeriesThumbnailType_Unknown;

        std::string sopClassUid;
        if (value.size() > 0 &&
            GetSopClassUid(sopClassUid, value[0]))
        {
          bool ok = true;
        
          for (Json::Value::ArrayIndex i = 1; i < value.size() && ok; i++)
          {
            std::string s;
            if (!GetSopClassUid(s, value[i]) ||
                s != sopClassUid)
            {
              ok = false;
            }
          }

          if (ok)
          {
            type = ExtractSopClassUid(sopClassUid);
          }
        }
      
        GetLoader()->AcquireThumbnail(GetSource(), GetStudyInstanceUid(),
                                      GetSeriesInstanceUid(), new Thumbnail(type));
      }
    }
  };


  class SeriesThumbnailsLoader::DicomWebThumbnailHandler : public SeriesThumbnailsLoader::Handler
  {
  public:
    DicomWebThumbnailHandler(boost::shared_ptr<SeriesThumbnailsLoader> loader,
                             const DicomSource& source,
                             const std::string& studyInstanceUid,
                             const std::string& seriesInstanceUid) :
      Handler(loader, source, studyInstanceUid, seriesInstanceUid)
    {
    }

    virtual void HandleSuccess(const std::string& body,
                               const std::map<std::string, std::string>& headers)
    {
      std::string mime = Orthanc::MIME_JPEG;
      for (std::map<std::string, std::string>::const_iterator
             it = headers.begin(); it != headers.end(); ++it)
      {
        if (boost::iequals(it->first, "content-type"))
        {
          mime = it->second;
        }
      }

      GetLoader()->AcquireThumbnail(GetSource(), GetStudyInstanceUid(),
                                    GetSeriesInstanceUid(), new Thumbnail(body, mime));
    }

    virtual void HandleError()
    {
      // The DICOMweb wasn't able to generate a thumbnail, try to
      // retrieve the SopClassUID tag using QIDO-RS

      std::map<std::string, std::string> arguments, headers;
      arguments["0020000D"] = GetStudyInstanceUid();
      arguments["0020000E"] = GetSeriesInstanceUid();
      arguments["includefield"] = "00080016";

      std::auto_ptr<IOracleCommand> command(
        GetSource().CreateDicomWebCommand(
          "/instances", arguments, headers, new DicomWebSopClassHandler(
            GetLoader(), GetSource(), GetStudyInstanceUid(), GetSeriesInstanceUid())));
      GetLoader()->Schedule(command.release());
    }
  };


  class SeriesThumbnailsLoader::ThumbnailInformation : public Orthanc::IDynamicObject
  {
  private:
    DicomSource  source_;
    std::string  studyInstanceUid_;
    std::string  seriesInstanceUid_;

  public:
    ThumbnailInformation(const DicomSource& source,
                         const std::string& studyInstanceUid,
                         const std::string& seriesInstanceUid) :
      source_(source),
      studyInstanceUid_(studyInstanceUid),
      seriesInstanceUid_(seriesInstanceUid)
    {
    }

    const DicomSource& GetDicomSource() const
    {
      return source_;
    }

    const std::string& GetStudyInstanceUid() const
    {
      return studyInstanceUid_;
    }

    const std::string& GetSeriesInstanceUid() const
    {
      return seriesInstanceUid_;
    }
  };


  class SeriesThumbnailsLoader::OrthancSopClassHandler : public SeriesThumbnailsLoader::Handler
  {
  private:
    std::string instanceId_;
      
  public:
    OrthancSopClassHandler(boost::shared_ptr<SeriesThumbnailsLoader> loader,
                           const DicomSource& source,
                           const std::string& studyInstanceUid,
                           const std::string& seriesInstanceUid,
                           const std::string& instanceId) :
      Handler(loader, source, studyInstanceUid, seriesInstanceUid),
      instanceId_(instanceId)
    {
    }

    virtual void HandleSuccess(const std::string& body,
                               const std::map<std::string, std::string>& headers)
    {
      SeriesThumbnailType type = ExtractSopClassUid(body);

      if (type == SeriesThumbnailType_Pdf ||
          type == SeriesThumbnailType_Video)
      {
        GetLoader()->AcquireThumbnail(GetSource(), GetStudyInstanceUid(),
                                      GetSeriesInstanceUid(), new Thumbnail(type));
      }
      else
      {
        std::auto_ptr<GetOrthancImageCommand> command(new GetOrthancImageCommand);
        command->SetUri("/instances/" + instanceId_ + "/preview");
        command->SetHttpHeader("Accept", Orthanc::MIME_JPEG);
        command->AcquirePayload(new ThumbnailInformation(
                                  GetSource(), GetStudyInstanceUid(), GetSeriesInstanceUid()));
        GetLoader()->Schedule(command.release());
      }
    }
  };


  class SeriesThumbnailsLoader::SelectOrthancInstanceHandler : public SeriesThumbnailsLoader::Handler
  {
  public:
    SelectOrthancInstanceHandler(boost::shared_ptr<SeriesThumbnailsLoader> loader,
                                 const DicomSource& source,
                                 const std::string& studyInstanceUid,
                                 const std::string& seriesInstanceUid) :
      Handler(loader, source, studyInstanceUid, seriesInstanceUid)
    {
    }

    virtual void HandleSuccess(const std::string& body,
                               const std::map<std::string, std::string>& headers)
    {
      static const char* const INSTANCES = "Instances";
      
      Json::Value json;
      Json::Reader reader;
      if (!reader.parse(body, json) ||
          json.type() != Json::objectValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
      }

      if (json.isMember(INSTANCES) &&
          json[INSTANCES].type() == Json::arrayValue &&
          json[INSTANCES].size() > 0)
      {
        // Select one instance of the series to generate the thumbnail
        Json::Value::ArrayIndex index = json[INSTANCES].size() / 2;
        if (json[INSTANCES][index].type() == Json::stringValue)
        {
          std::map<std::string, std::string> arguments, headers;
          arguments["quality"] = boost::lexical_cast<std::string>(JPEG_QUALITY);
          headers["Accept"] = Orthanc::MIME_JPEG;

          const std::string instance = json[INSTANCES][index].asString();

          std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
          command->SetUri("/instances/" + instance + "/metadata/SopClassUid");
          command->AcquirePayload(
            new OrthancSopClassHandler(
              GetLoader(), GetSource(), GetStudyInstanceUid(), GetSeriesInstanceUid(), instance));
          GetLoader()->Schedule(command.release());
        }
      }
    }
  };

    
  void SeriesThumbnailsLoader::Schedule(IOracleCommand* command)
  {
    std::auto_ptr<ILoadersContext::ILock> lock(context_.Lock());
    lock->Schedule(GetSharedObserver(), priority_, command);
  }    

  
  void SeriesThumbnailsLoader::Handle(const HttpCommand::SuccessMessage& message)
  {
    assert(message.GetOrigin().HasPayload());
    dynamic_cast<Handler&>(message.GetOrigin().GetPayload()).HandleSuccess(message.GetAnswer(), message.GetAnswerHeaders());
  }


  void SeriesThumbnailsLoader::Handle(const OrthancRestApiCommand::SuccessMessage& message)
  {
    assert(message.GetOrigin().HasPayload());
    dynamic_cast<Handler&>(message.GetOrigin().GetPayload()).HandleSuccess(message.GetAnswer(), message.GetAnswerHeaders());
  }


  void SeriesThumbnailsLoader::Handle(const GetOrthancImageCommand::SuccessMessage& message)
  {
    assert(message.GetOrigin().HasPayload());

    std::auto_ptr<Orthanc::ImageAccessor> resized(Orthanc::ImageProcessing::FitSize(message.GetImage(), width_, height_));

    std::string jpeg;
    Orthanc::JpegWriter writer;
    writer.SetQuality(JPEG_QUALITY);
    writer.WriteToMemory(jpeg, *resized);

    const ThumbnailInformation& info = dynamic_cast<ThumbnailInformation&>(message.GetOrigin().GetPayload());
    AcquireThumbnail(info.GetDicomSource(), info.GetStudyInstanceUid(),
                     info.GetSeriesInstanceUid(), new Thumbnail(jpeg, Orthanc::MIME_JPEG));      
  }


  void SeriesThumbnailsLoader::Handle(const OracleCommandExceptionMessage& message)
  {
    const OracleCommandBase& command = dynamic_cast<const OracleCommandBase&>(message.GetOrigin());
    assert(command.HasPayload());
    dynamic_cast<Handler&>(command.GetPayload()).HandleError();
  }


  SeriesThumbnailsLoader::SeriesThumbnailsLoader(ILoadersContext& context,
                                                 int priority) :
    context_(context),
    priority_(priority),
    width_(128),
    height_(128)
  {
  }
    
  
  boost::shared_ptr<IObserver> SeriesThumbnailsLoader::Factory::Create(ILoadersContext::ILock& stone)
  {
    boost::shared_ptr<SeriesThumbnailsLoader> result(new SeriesThumbnailsLoader(stone.GetContext(), priority_));
    result->Register<GetOrthancImageCommand::SuccessMessage>(stone.GetOracleObservable(), &SeriesThumbnailsLoader::Handle);
    result->Register<HttpCommand::SuccessMessage>(stone.GetOracleObservable(), &SeriesThumbnailsLoader::Handle);
    result->Register<OracleCommandExceptionMessage>(stone.GetOracleObservable(), &SeriesThumbnailsLoader::Handle);
    result->Register<OrthancRestApiCommand::SuccessMessage>(stone.GetOracleObservable(), &SeriesThumbnailsLoader::Handle);
    return result;
  }


  void SeriesThumbnailsLoader::SetThumbnailSize(unsigned int width,
                                                unsigned int height)
  {
    if (width <= 0 ||
        height <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      width_ = width;
      height_ = height;
    }
  }

    
  void SeriesThumbnailsLoader::Clear()
  {
    for (Thumbnails::iterator it = thumbnails_.begin(); it != thumbnails_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }

    thumbnails_.clear();
  }

    
  SeriesThumbnailType SeriesThumbnailsLoader::GetSeriesThumbnail(std::string& image,
                                                                 std::string& mime,
                                                                 const std::string& seriesInstanceUid) const
  {
    Thumbnails::const_iterator found = thumbnails_.find(seriesInstanceUid);

    if (found == thumbnails_.end())
    {
      return SeriesThumbnailType_Unknown;
    }
    else
    {
      assert(found->second != NULL);
      image.assign(found->second->GetImage());
      mime.assign(found->second->GetMime());
      return found->second->GetType();
    }
  }


  void SeriesThumbnailsLoader::ScheduleLoadThumbnail(const DicomSource& source,
                                                     const std::string& patientId,
                                                     const std::string& studyInstanceUid,
                                                     const std::string& seriesInstanceUid)
  {
    if (source.IsDicomWeb())
    {
      if (!source.HasDicomWebRendered())
      {
        // TODO - Could use DCMTK here
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol,
                                        "DICOMweb server is not able to generate renderings of DICOM series");
      }
      
      const std::string uri = ("/studies/" + studyInstanceUid +
                               "/series/" + seriesInstanceUid + "/rendered");

      std::map<std::string, std::string> arguments, headers;
      arguments["viewport"] = (boost::lexical_cast<std::string>(width_) + "," +
                               boost::lexical_cast<std::string>(height_));

      // Needed to set this header explicitly, as long as emscripten
      // does not include macro "EMSCRIPTEN_FETCH_RESPONSE_HEADERS"
      // https://github.com/emscripten-core/emscripten/pull/8486
      headers["Accept"] = Orthanc::MIME_JPEG;

      std::auto_ptr<IOracleCommand> command(
        source.CreateDicomWebCommand(
          uri, arguments, headers, new DicomWebThumbnailHandler(
            shared_from_this(), source, studyInstanceUid, seriesInstanceUid)));
      Schedule(command.release());
    }
    else if (source.IsOrthanc())
    {
      // Dummy SOP Instance UID, as we are working at the "series" level
      Orthanc::DicomInstanceHasher hasher(patientId, studyInstanceUid, seriesInstanceUid, "dummy");

      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetUri("/series/" + hasher.HashSeries());
      command->AcquirePayload(new SelectOrthancInstanceHandler(
                                shared_from_this(), source, studyInstanceUid, seriesInstanceUid));
      Schedule(command.release());
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                      "Can only load thumbnails from Orthanc or DICOMweb");
    }
  }
}
