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

#include "ILayerRenderer.h"
#include "../Toolbox/Slice.h"

namespace OrthancStone
{
  class ILayerSource : public boost::noncopyable
  {
  public:
    class IObserver : public boost::noncopyable
    {
    public:
      virtual ~IObserver()
      {
      }

      // Triggered as soon as the source has enough information to
      // answer to "GetExtent()"
      virtual void NotifyGeometryReady(ILayerSource& source) = 0;
      
      virtual void NotifyGeometryError(ILayerSource& source) = 0;
      
      // Triggered if the extent or the content of the volume has changed
      virtual void NotifySourceChange(ILayerSource& source) = 0;

      // Triggered if the content of some slice in the source volume has changed
      virtual void NotifySliceChange(ILayerSource& source,
                                     const Slice& slice) = 0;

      // The layer must be deleted by the observer. "layer" will never
      // be "NULL", otherwise "NotifyLayerError()" would have been
      // called.
      virtual void NotifyLayerReady(ILayerRenderer *layer,
                                    ILayerSource& source,
                                    const Slice& slice) = 0;

      virtual void NotifyLayerError(ILayerSource& source,
                                    const SliceGeometry& slice) = 0;
    };
    
    virtual ~ILayerSource()
    {
    }

    virtual void SetObserver(IObserver& observer) = 0;

    virtual bool GetExtent(double& x1,
                           double& y1,
                           double& x2,
                           double& y2,
                           const SliceGeometry& viewportSlice) = 0;

    virtual void ScheduleLayerCreation(const SliceGeometry& viewportSlice) = 0;

    virtual void Start() = 0;
  };
}
