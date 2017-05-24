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

#include "LayerSourceBase.h"
#include "../Toolbox/IWebService.h"
#include "../Toolbox/IVolumeSlicesObserver.h"
#include "../Toolbox/OrthancSlicesLoader.h"

namespace OrthancStone
{  
  class OrthancFrameLayerSource :
    public LayerSourceBase,
    private OrthancSlicesLoader::ICallback
  {
  private:
    std::string             instanceId_;
    unsigned int            frame_;
    OrthancSlicesLoader     loader_;
    IVolumeSlicesObserver*  observer2_;

    virtual void NotifyGeometryReady(const OrthancSlicesLoader& loader);

    virtual void NotifyGeometryError(const OrthancSlicesLoader& loader);

    virtual void NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice,
                                       Orthanc::ImageAccessor* image);

    virtual void NotifySliceImageError(const OrthancSlicesLoader& loader,
                                       unsigned int sliceIndex,
                                       const Slice& slice);

  protected:
    virtual void StartInternal();

  public:
    using LayerSourceBase::SetObserver;

    OrthancFrameLayerSource(IWebService& orthanc,
                            const std::string& instanceId,
                            unsigned int frame);

    void SetObserver(IVolumeSlicesObserver& observer);

    virtual bool GetExtent(double& x1,
                           double& y1,
                           double& x2,
                           double& y2,
                           const SliceGeometry& viewportSlice /* ignored */);

    virtual void ScheduleLayerCreation(const SliceGeometry& viewportSlice);
  };
}
