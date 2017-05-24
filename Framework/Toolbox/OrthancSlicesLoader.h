/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class OrthancSlicesLoader : public boost::noncopyable
  {
  public:
    class ICallback : public boost::noncopyable
    {
    public:
      virtual ~ICallback()
      {
      }

      virtual void NotifyGeometryReady(const OrthancSlicesLoader& loader) = 0;

      virtual void NotifyGeometryError(const OrthancSlicesLoader& loader) = 0;

      virtual void NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                         unsigned int sliceIndex,
                                         const Slice& slice,
                                         Orthanc::ImageAccessor* image) = 0;

      virtual void NotifySliceImageError(const OrthancSlicesLoader& loader,
                                         unsigned int sliceIndex,
                                         const Slice& slice) = 0;
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
      Mode_LoadImage
    };

    class Operation;
    class WebCallback;

    boost::shared_ptr<WebCallback>  webCallback_;  // This is a PImpl pattern

    ICallback&    userCallback_;
    IWebService&  orthanc_;
    State         state_;
    SlicesSorter  slices_;


    void ParseSeriesGeometry(const void* answer,
                             size_t size);

    void ParseInstanceGeometry(const std::string& instanceId,
                               unsigned int frame,
                               const void* answer,
                               size_t size);

    void ParseSliceImage(const Operation& operation,
                         const void* answer,
                         size_t size);
    
    
  public:
    OrthancSlicesLoader(ICallback& callback,
                        IWebService& orthanc);

    void ScheduleLoadSeries(const std::string& seriesId);

    void ScheduleLoadInstance(const std::string& instanceId,
                              unsigned int frame);

    size_t GetSliceCount() const;

    const Slice& GetSlice(size_t index) const;

    void ScheduleLoadSliceImage(size_t index);
  };
}
