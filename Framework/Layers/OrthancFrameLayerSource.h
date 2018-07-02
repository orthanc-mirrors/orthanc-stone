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

#include "LayerSourceBase.h"
#include "../Toolbox/IWebService.h"
#include "../Toolbox/OrthancSlicesLoader.h"

namespace OrthancStone
{  
  class OrthancFrameLayerSource :
    public LayerSourceBase,
    private OrthancSlicesLoader::ICallback
  {
  private:
    OrthancSlicesLoader  loader_;
    SliceImageQuality    quality_;

    virtual void NotifyGeometryReady(const OrthancSlicesLoader& loader);

    virtual void NotifyGeometryError(const OrthancSlicesLoader& loader);

    virtual void NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       std::auto_ptr<Orthanc::ImageAccessor>& image,
                                       SliceImageQuality quality);

    virtual void NotifySliceImageError(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       SliceImageQuality quality);

  public:
    OrthancFrameLayerSource(MessageBroker& broker, IWebService& orthanc);

    void LoadSeries(const std::string& seriesId);

    void LoadInstance(const std::string& instanceId);

    void LoadFrame(const std::string& instanceId,
                   unsigned int frame);

    void SetImageQuality(SliceImageQuality quality)
    {
      quality_ = quality;
    }

    size_t GetSliceCount() const
    {
      return loader_.GetSliceCount();
    }

    const Slice& GetSlice(size_t slice) const 
    {
      return loader_.GetSlice(slice);
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice);

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice);
  };
}
