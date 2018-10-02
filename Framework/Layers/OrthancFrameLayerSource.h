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
#include "../Toolbox/OrthancApiClient.h"

namespace OrthancStone
{  
  // this class is in charge of loading a Frame.
  // once it's been loaded (first the geometry and then the image),
  // messages are sent to observers so they can use it
  class OrthancFrameLayerSource :
    public LayerSourceBase,
    public IObserver
    //private OrthancSlicesLoader::ISliceLoaderObserver
  {
  private:
    OrthancSlicesLoader  loader_;
    SliceImageQuality    quality_;

  public:
    OrthancFrameLayerSource(MessageBroker& broker, OrthancApiClient& orthanc);

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

protected:
    void OnSliceGeometryReady(const OrthancSlicesLoader::SliceGeometryReadyMessage& message);
    void OnSliceGeometryError(const OrthancSlicesLoader::SliceGeometryErrorMessage& message);
    void OnSliceImageReady(const OrthancSlicesLoader::SliceImageReadyMessage& message);
    void OnSliceImageError(const OrthancSlicesLoader::SliceImageErrorMessage& message);
//    virtual void HandleMessage(IObservable& from, const IMessage& message);
  };
}
