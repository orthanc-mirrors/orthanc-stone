/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include "IWebService.h"
#include "SlicesSorter.h"
#include "../StoneEnumerations.h"

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class OrthancSlicesLoader : public boost::noncopyable
  {
  public:
    struct SliceImageReadyMessage : public IMessage
    {
      unsigned int sliceIndex_;
      const Slice& slice_;
      std::auto_ptr<Orthanc::ImageAccessor>& image_;
      SliceImageQuality effectiveQuality_;

      SliceImageReadyMessage(unsigned int sliceIndex,
                        const Slice& slice,
                        std::auto_ptr<Orthanc::ImageAccessor>& image,
                        SliceImageQuality effectiveQuality)
        : IMessage(MessageType_SliceImageReady),
          sliceIndex_(sliceIndex),
          slice_(slice),
          image_(image),
          effectiveQuality_(effectiveQuality)
      {
      }
    };

    struct SliceImageErrorMessage : public IMessage
    {
      const Slice& slice_;
      unsigned int sliceIndex_;
      SliceImageQuality effectiveQuality_;

      SliceImageErrorMessage(unsigned int sliceIndex,
                        const Slice& slice,
                        SliceImageQuality effectiveQuality)
        : IMessage(MessageType_SliceImageError),
          slice_(slice),
          sliceIndex_(sliceIndex),
          effectiveQuality_(effectiveQuality)
      {
      }
    };

  public:
    class ISliceLoaderObserver : public IObserver
    {
    public:

      ISliceLoaderObserver(MessageBroker& broker)
        : IObserver(broker)
      {
      }

      virtual ~ISliceLoaderObserver()
      {
      }

      virtual void HandleMessage(IObservable& from, const IMessage& message);

      virtual void OnSliceGeometryReady(const OrthancSlicesLoader& loader) = 0;

      virtual void OnSliceGeometryError(const OrthancSlicesLoader& loader) = 0;

      virtual void OnSliceImageReady(const OrthancSlicesLoader& loader,
                                         unsigned int sliceIndex,
                                         const Slice& slice,
                                         std::auto_ptr<Orthanc::ImageAccessor>& image,
                                         SliceImageQuality effectiveQuality) = 0;

      virtual void OnSliceImageError(const OrthancSlicesLoader& loader,
                                         unsigned int sliceIndex,
                                         const Slice& slice,
                                         SliceImageQuality quality) = 0;
    };
    
  private:
    enum State
    {
      State_Error,
      State_Initialization,
      State_LoadingGeometry,
      State_GeometryReady
    };
    
    enum Mode
    {
      Mode_SeriesGeometry,
      Mode_InstanceGeometry,
      Mode_FrameGeometry,
      Mode_LoadImage,
      Mode_LoadRawImage,
      Mode_LoadDicomFile
    };

    class Operation;
    class WebCallback;

    boost::shared_ptr<WebCallback>  webCallback_;  // This is a PImpl pattern

    ISliceLoaderObserver&    userCallback_;
    IWebService&  orthanc_;
    State         state_;
    SlicesSorter  slices_;

    void NotifySliceImageSuccess(const Operation& operation,
                                 std::auto_ptr<Orthanc::ImageAccessor>& image) const;
  
    void NotifySliceImageError(const Operation& operation) const;
    
    void ParseSeriesGeometry(const void* answer,
                             size_t size);

    void ParseInstanceGeometry(const std::string& instanceId,
                               const void* answer,
                               size_t size);

    void ParseFrameGeometry(const std::string& instanceId,
                            unsigned int frame,
                            const void* answer,
                            size_t size);

    void ParseSliceImagePng(const Operation& operation,
                            const void* answer,
                            size_t size);

    void ParseSliceImagePam(const Operation& operation,
                            const void* answer,
                            size_t size);

    void ParseSliceImageJpeg(const Operation& operation,
                             const void* answer,
                             size_t size);

    void ParseSliceRawImage(const Operation& operation,
                            const void* answer,
                            size_t size);

    void ScheduleSliceImagePng(const Slice& slice,
                               size_t index);

    void ScheduleSliceImagePam(const Slice& slice,
                               size_t index);

    void ScheduleSliceImageJpeg(const Slice& slice,
                                size_t index,
                                SliceImageQuality quality);

    void SortAndFinalizeSlices();
    
  public:
    OrthancSlicesLoader(MessageBroker& broker,
                        ISliceLoaderObserver& callback,
                        IWebService& orthanc);

    void ScheduleLoadSeries(const std::string& seriesId);

    void ScheduleLoadInstance(const std::string& instanceId);

    void ScheduleLoadFrame(const std::string& instanceId,
                           unsigned int frame);

    bool IsGeometryReady() const;

    size_t GetSliceCount() const;

    const Slice& GetSlice(size_t index) const;

    bool LookupSlice(size_t& index,
                     const CoordinateSystem3D& plane) const;

    void ScheduleLoadSliceImage(size_t index,
                                SliceImageQuality requestedQuality);
  };
}
