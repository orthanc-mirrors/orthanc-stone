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
      virtual void NotifyGeometryReady(const ILayerSource& source) = 0;
      
      virtual void NotifyGeometryError(const ILayerSource& source) = 0;
      
      // Triggered if the content of several slices in the source
      // volume has changed
      virtual void NotifyContentChange(const ILayerSource& source) = 0;

      // Triggered if the content of some individual slice in the
      // source volume has changed
      virtual void NotifySliceChange(const ILayerSource& source,
                                     const Slice& slice) = 0;
 
      // The layer must be deleted by the observer that releases the
      // std::auto_ptr
      virtual void NotifyLayerReady(std::auto_ptr<ILayerRenderer>& layer,
                                    const ILayerSource& source,
                                    const CoordinateSystem3D& slice,
                                    bool isError) = 0;  // TODO Shouldn't this be separate as NotifyLayerError?
    };
    
    virtual ~ILayerSource()
    {
    }

    virtual void Register(IObserver& observer) = 0;

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice) = 0;

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice) = 0;
  };
}
