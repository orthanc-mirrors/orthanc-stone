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


#include "OrthancFrameLayerSource.h"

#include "FrameRenderer.h"
#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  void OrthancFrameLayerSource::NotifyGeometryReady(const OrthancSlicesLoader& loader)
  {
    if (loader.GetSliceCount() > 0)
    {
      LayerSourceBase::NotifyGeometryReady();
    }
    else
    {
      LayerSourceBase::NotifyGeometryError();
    }
  }

  void OrthancFrameLayerSource::NotifyGeometryError(const OrthancSlicesLoader& loader)
  {
    LayerSourceBase::NotifyGeometryError();
  }

  void OrthancFrameLayerSource::NotifySliceImageReady(const OrthancSlicesLoader& loader,
                                                      unsigned int sliceIndex,
                                                      const Slice& slice,
                                                      std::auto_ptr<Orthanc::ImageAccessor>& image,
                                                      SliceImageQuality quality)
  {
    bool isFull = (quality == SliceImageQuality_Full);
    LayerSourceBase::NotifyLayerReady(FrameRenderer::CreateRenderer(image.release(), slice, isFull),
                                      slice, false);
  }

  void OrthancFrameLayerSource::NotifySliceImageError(const OrthancSlicesLoader& loader,
                                                      unsigned int sliceIndex,
                                                      const Slice& slice,
                                                      SliceImageQuality quality)
  {
    LayerSourceBase::NotifyLayerReady(NULL, slice, true);
  }


  OrthancFrameLayerSource::OrthancFrameLayerSource(IWebService& orthanc) :
    loader_(*this, orthanc),
    quality_(SliceImageQuality_Full)
  {
  }

  
  void OrthancFrameLayerSource::LoadInstance(const std::string& instanceId,
                                             unsigned int frame)
  {
    loader_.ScheduleLoadInstance(instanceId, frame);
  }


  void OrthancFrameLayerSource::LoadSeries(const std::string& seriesId)
  {
    loader_.ScheduleLoadSeries(seriesId);
  }


  bool OrthancFrameLayerSource::GetExtent(std::vector<Vector>& points,
                                          const SliceGeometry& viewportSlice)
  {
    size_t index;
    if (loader_.IsGeometryReady() &&
        loader_.LookupSlice(index, viewportSlice))
    {
      loader_.GetSlice(index).GetExtent(points);
      return true;
    }
    else
    {
      return false;
    }
  }

  
  void OrthancFrameLayerSource::ScheduleLayerCreation(const SliceGeometry& viewportSlice)
  {
    size_t index;

    if (loader_.IsGeometryReady())
    {
      if (loader_.LookupSlice(index, viewportSlice))
      {
        loader_.ScheduleLoadSliceImage(index, quality_);
      }
      else
      {
        Slice slice;
        LayerSourceBase::NotifyLayerReady(NULL, slice, true);
      }
    }
  }
}
