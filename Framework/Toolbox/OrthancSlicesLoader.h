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

#include "../Messages/IObservable.h"
#include "../StoneEnumerations.h"
#include "IWebService.h"
#include "OrthancApiClient.h"
#include "SlicesSorter.h"

#include <Core/Images/Image.h>


namespace OrthancStone
{
  class OrthancSlicesLoader : public IObservable, public IObserver
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, SliceGeometryReadyMessage, OrthancSlicesLoader);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, SliceGeometryErrorMessage, OrthancSlicesLoader);

    
    class SliceImageReadyMessage : public OriginMessage<OrthancSlicesLoader>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      unsigned int                   sliceIndex_;
      const Slice&                   slice_;
      const Orthanc::ImageAccessor&  image_;
      SliceImageQuality              effectiveQuality_;

    public:
      SliceImageReadyMessage(const OrthancSlicesLoader& origin,
                             unsigned int sliceIndex,
                             const Slice& slice,
                             const Orthanc::ImageAccessor& image,
                             SliceImageQuality effectiveQuality) :
        OriginMessage(origin),
        sliceIndex_(sliceIndex),
        slice_(slice),
        image_(image),
        effectiveQuality_(effectiveQuality)
      {
      }

      unsigned int GetSliceIndex() const
      {
        return sliceIndex_;
      }

      const Slice& GetSlice() const
      {
        return slice_;
      }

      const Orthanc::ImageAccessor& GetImage() const
      {
        return image_;
      }

      SliceImageQuality GetEffectiveQuality() const
      {
        return effectiveQuality_;
      }        
    };
    

    class SliceImageErrorMessage : public OriginMessage<OrthancSlicesLoader>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      const Slice&       slice_;
      unsigned int       sliceIndex_;
      SliceImageQuality  effectiveQuality_;

    public:
      SliceImageErrorMessage(const OrthancSlicesLoader& origin,
                             unsigned int sliceIndex,
                             const Slice& slice,
                             SliceImageQuality effectiveQuality) :
        OriginMessage(origin),
        slice_(slice),
        sliceIndex_(sliceIndex),
        effectiveQuality_(effectiveQuality)
      {
      }
      unsigned int GetSliceIndex() const
      {
        return sliceIndex_;
      }

      const Slice& GetSlice() const
      {
        return slice_;
      }

      SliceImageQuality GetEffectiveQuality() const
      {
        return effectiveQuality_;
      }        
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

    OrthancApiClient&  orthanc_;
    State         state_;
    SlicesSorter  slices_;

    void NotifySliceImageSuccess(const Operation& operation,
                                 const Orthanc::ImageAccessor& image);
    
    void NotifySliceImageError(const Operation& operation);

    void OnGeometryError(const IWebService::HttpRequestErrorMessage& message);

    void OnSliceImageError(const IWebService::HttpRequestErrorMessage& message);

    void ParseSeriesGeometry(const OrthancApiClient::JsonResponseReadyMessage& message);

    void ParseInstanceGeometry(const OrthancApiClient::JsonResponseReadyMessage& message);

    void ParseFrameGeometry(const OrthancApiClient::JsonResponseReadyMessage& message);

    void ParseSliceImagePng(const OrthancApiClient::BinaryResponseReadyMessage& message);

    void ParseSliceImagePam(const OrthancApiClient::BinaryResponseReadyMessage& message);

    void ParseSliceImageJpeg(const OrthancApiClient::JsonResponseReadyMessage& message);

    void ParseSliceRawImage(const OrthancApiClient::BinaryResponseReadyMessage& message);

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
                        //ISliceLoaderObserver& callback,
                        OrthancApiClient& orthancApi);

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
