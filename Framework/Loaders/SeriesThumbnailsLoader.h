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


#pragma once

#include "../Oracle/GetOrthancImageCommand.h"
#include "../Oracle/HttpCommand.h"
#include "../Oracle/OracleCommandExceptionMessage.h"
#include "../Oracle/OrthancRestApiCommand.h"
#include "DicomSource.h"
#include "ILoaderFactory.h"
#include "OracleScheduler.h"


namespace OrthancStone
{
  enum SeriesThumbnailType
  {
    SeriesThumbnailType_NotLoaded = 1,  // The remote server cannot decode this image
    SeriesThumbnailType_Unsupported = 2,  // The remote server cannot decode this image
    SeriesThumbnailType_Pdf = 3,
    SeriesThumbnailType_Video = 4,
    SeriesThumbnailType_Image = 5
  };
  

  class SeriesThumbnailsLoader :
    public IObservable,
    public ObserverBase<SeriesThumbnailsLoader>
  {
  private:
    class Thumbnail : public boost::noncopyable
    {
    private:
      SeriesThumbnailType  type_;
      std::string          image_;
      std::string          mime_;

    public:
      Thumbnail(const std::string& image,
                const std::string& mime);

      Thumbnail(SeriesThumbnailType type);

      SeriesThumbnailType GetType() const
      {
        return type_;
      }

      const std::string& GetImage() const
      {
        return image_;
      }

      const std::string& GetMime() const
      {
        return mime_;
      }
    };

  public:
    class ThumbnailLoadedMessage : public OriginMessage<SeriesThumbnailsLoader>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      const DicomSource&   source_;
      const std::string&   studyInstanceUid_;
      const std::string&   seriesInstanceUid_;
      const Thumbnail&     thumbnail_;

    public:
      ThumbnailLoadedMessage(const SeriesThumbnailsLoader& origin,
                             const DicomSource& source,
                             const std::string& studyInstanceUid,
                             const std::string& seriesInstanceUid,
                             const Thumbnail& thumbnail) :
        OriginMessage(origin),
        source_(source),
        studyInstanceUid_(studyInstanceUid),
        seriesInstanceUid_(seriesInstanceUid),
        thumbnail_(thumbnail)
      {
      }

      const DicomSource& GetDicomSource() const
      {
        return source_;
      }

      SeriesThumbnailType GetType() const
      {
        return thumbnail_.GetType();
      }

      const std::string& GetStudyInstanceUid() const
      {
        return studyInstanceUid_;
      }

      const std::string& GetSeriesInstanceUid() const
      {
        return seriesInstanceUid_;
      }

      const std::string& GetEncodedImage() const
      {
        return thumbnail_.GetImage();
      }

      const std::string& GetMime() const
      {
        return thumbnail_.GetMime();
      }
    };

  private:
    class Handler;
    class DicomWebSopClassHandler;
    class DicomWebThumbnailHandler;
    class ThumbnailInformation;
    class OrthancSopClassHandler;
    class SelectOrthancInstanceHandler;
    
    // Maps a "Series Instance UID" to a thumbnail
    typedef std::map<std::string, Thumbnail*>  Thumbnails;

    ILoadersContext&  context_;
    Thumbnails      thumbnails_;
    int             priority_;
    unsigned int    width_;
    unsigned int    height_;

    void AcquireThumbnail(const DicomSource& source,
                          const std::string& studyInstanceUid,
                          const std::string& seriesInstanceUid,
                          Thumbnail* thumbnail /* takes ownership */);

    void Schedule(IOracleCommand* command);
  
    void Handle(const HttpCommand::SuccessMessage& message);

    void Handle(const OrthancRestApiCommand::SuccessMessage& message);

    void Handle(const GetOrthancImageCommand::SuccessMessage& message);

    void Handle(const OracleCommandExceptionMessage& message);

    SeriesThumbnailsLoader(ILoadersContext& context,
                           int priority);
    
  public:
    class Factory : public ILoaderFactory
    {
    private:
      int priority_;

    public:
      Factory() :
        priority_(0)
      {
      }

      void SetPriority(int priority)
      {
        priority_ = priority;
      }

      virtual boost::shared_ptr<IObserver> Create(ILoadersContext::ILock& context);
    };


    virtual ~SeriesThumbnailsLoader()
    {
      Clear();
    }

    void SetThumbnailSize(unsigned int width,
                          unsigned int height);
    
    void Clear();
    
    SeriesThumbnailType GetSeriesThumbnail(std::string& image,
                                           std::string& mime,
                                           const std::string& seriesInstanceUid) const;

    void ScheduleLoadThumbnail(const DicomSource& source,
                               const std::string& patientId,
                               const std::string& studyInstanceUid,
                               const std::string& seriesInstanceUid);
  };
}
