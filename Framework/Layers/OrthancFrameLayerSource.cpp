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


#include "OrthancFrameLayerSource.h"

#include "FrameRenderer.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>

namespace OrthancStone
{

  void OrthancFrameLayerSource::OnSliceGeometryReady(const OrthancSlicesLoader::SliceGeometryReadyMessage& message)
  {
    if (message.origin_.GetSliceCount() > 0)
    {
      LayerSourceBase::NotifyGeometryReady();
    }
    else
    {
      LayerSourceBase::NotifyGeometryError();
    }
  }

  void OrthancFrameLayerSource::OnSliceGeometryError(const OrthancSlicesLoader::SliceGeometryErrorMessage& message)
  {
    LayerSourceBase::NotifyGeometryError();
  }

  void OrthancFrameLayerSource::OnSliceImageReady(const OrthancSlicesLoader::SliceImageReadyMessage& message)
  {
    // first notify that the image is ready (targeted to, i.e: an image cache)
    LayerSourceBase::NotifyImageReady(message.image_, message.effectiveQuality_, message.slice_);

    // then notify that the layer is ready for render
    bool isFull = (message.effectiveQuality_ == SliceImageQuality_FullPng || message.effectiveQuality_ == SliceImageQuality_FullPam);
    std::auto_ptr<Orthanc::ImageAccessor> accessor(new Orthanc::ImageAccessor());
    message.image_->GetReadOnlyAccessor(*accessor);

    LayerSourceBase::NotifyLayerReady(FrameRenderer::CreateRenderer(accessor.release(), message.slice_, isFull),
                                      message.slice_.GetGeometry(), false);

  }

  void OrthancFrameLayerSource::OnSliceImageError(const OrthancSlicesLoader::SliceImageErrorMessage& message)
  {
    LayerSourceBase::NotifyLayerReady(NULL, message.slice_.GetGeometry(), true);
  }

  OrthancFrameLayerSource::OrthancFrameLayerSource(MessageBroker& broker, OrthancApiClient& orthanc) :
    LayerSourceBase(broker),
    IObserver(broker),
    loader_(broker, orthanc),
    quality_(SliceImageQuality_FullPng)
  {
    loader_.RegisterObserverCallback(new Callable<OrthancFrameLayerSource, OrthancSlicesLoader::SliceGeometryReadyMessage>(*this, &OrthancFrameLayerSource::OnSliceGeometryReady));
    loader_.RegisterObserverCallback(new Callable<OrthancFrameLayerSource, OrthancSlicesLoader::SliceGeometryErrorMessage>(*this, &OrthancFrameLayerSource::OnSliceGeometryError));
    loader_.RegisterObserverCallback(new Callable<OrthancFrameLayerSource, OrthancSlicesLoader::SliceImageReadyMessage>(*this, &OrthancFrameLayerSource::OnSliceImageReady));
    loader_.RegisterObserverCallback(new Callable<OrthancFrameLayerSource, OrthancSlicesLoader::SliceImageErrorMessage>(*this, &OrthancFrameLayerSource::OnSliceImageError));
  }

  
  void OrthancFrameLayerSource::LoadSeries(const std::string& seriesId)
  {
    loader_.ScheduleLoadSeries(seriesId);
  }


  void OrthancFrameLayerSource::LoadInstance(const std::string& instanceId)
  {
    loader_.ScheduleLoadInstance(instanceId);
  }


  void OrthancFrameLayerSource::LoadFrame(const std::string& instanceId,
                                          unsigned int frame)
  {
    loader_.ScheduleLoadFrame(instanceId, frame);
  }


  bool OrthancFrameLayerSource::GetExtent(std::vector<Vector>& points,
                                          const CoordinateSystem3D& viewportSlice)
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

  
  void OrthancFrameLayerSource::ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice)
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
        LayerSourceBase::NotifyLayerReady(NULL, slice.GetGeometry(), true);
      }
    }
  }
}
