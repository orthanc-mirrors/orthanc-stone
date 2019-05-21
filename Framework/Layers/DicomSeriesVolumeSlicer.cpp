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


#include "DicomSeriesVolumeSlicer.h"

#include "FrameRenderer.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/lexical_cast.hpp>

namespace Deprecated
{

  void DicomSeriesVolumeSlicer::OnSliceGeometryReady(const OrthancSlicesLoader::SliceGeometryReadyMessage& message)
  {
    if (message.GetOrigin().GetSlicesCount() > 0)
    {
      BroadcastMessage(IVolumeSlicer::GeometryReadyMessage(*this));
    }
    else
    {
      BroadcastMessage(IVolumeSlicer::GeometryErrorMessage(*this));
    }
  }

  void DicomSeriesVolumeSlicer::OnSliceGeometryError(const OrthancSlicesLoader::SliceGeometryErrorMessage& message)
  {
    BroadcastMessage(IVolumeSlicer::GeometryErrorMessage(*this));
  }


  class DicomSeriesVolumeSlicer::RendererFactory : public LayerReadyMessage::IRendererFactory
  {
  private:
    const OrthancSlicesLoader::SliceImageReadyMessage&  message_;

  public:
    RendererFactory(const OrthancSlicesLoader::SliceImageReadyMessage& message) :
      message_(message)
    {
    }

    virtual ILayerRenderer* CreateRenderer() const
    {
      bool isFull = (message_.GetEffectiveQuality() == OrthancStone::SliceImageQuality_FullPng ||
                     message_.GetEffectiveQuality() == OrthancStone::SliceImageQuality_FullPam);

      return FrameRenderer::CreateRenderer(message_.GetImage(), message_.GetSlice(), isFull);
    }
  };

  void DicomSeriesVolumeSlicer::OnSliceImageReady(const OrthancSlicesLoader::SliceImageReadyMessage& message)
  {
    // first notify that the pixel data of the frame is ready (targeted to, i.e: an image cache)
    BroadcastMessage(FrameReadyMessage(*this, message.GetImage(), 
                                  message.GetEffectiveQuality(), message.GetSlice()));

    // then notify that the layer is ready for rendering
    RendererFactory factory(message);
    BroadcastMessage(IVolumeSlicer::LayerReadyMessage(*this, factory, message.GetSlice().GetGeometry()));
  }

  void DicomSeriesVolumeSlicer::OnSliceImageError(const OrthancSlicesLoader::SliceImageErrorMessage& message)
  {
    BroadcastMessage(IVolumeSlicer::LayerErrorMessage(*this, message.GetSlice().GetGeometry()));
  }


  DicomSeriesVolumeSlicer::DicomSeriesVolumeSlicer(OrthancStone::MessageBroker& broker,
                                                   OrthancApiClient& orthanc) :
    IVolumeSlicer(broker),
    IObserver(broker),
    loader_(broker, orthanc),
    quality_(OrthancStone::SliceImageQuality_FullPng)
  {
    loader_.RegisterObserverCallback(
      new OrthancStone::Callable<DicomSeriesVolumeSlicer, OrthancSlicesLoader::SliceGeometryReadyMessage>
        (*this, &DicomSeriesVolumeSlicer::OnSliceGeometryReady));

    loader_.RegisterObserverCallback(
      new OrthancStone::Callable<DicomSeriesVolumeSlicer, OrthancSlicesLoader::SliceGeometryErrorMessage>
      (*this, &DicomSeriesVolumeSlicer::OnSliceGeometryError));

    loader_.RegisterObserverCallback(
      new OrthancStone::Callable<DicomSeriesVolumeSlicer, OrthancSlicesLoader::SliceImageReadyMessage>
        (*this, &DicomSeriesVolumeSlicer::OnSliceImageReady));

    loader_.RegisterObserverCallback(
      new OrthancStone::Callable<DicomSeriesVolumeSlicer, OrthancSlicesLoader::SliceImageErrorMessage>
      (*this, &DicomSeriesVolumeSlicer::OnSliceImageError));
  }

  
  void DicomSeriesVolumeSlicer::LoadSeries(const std::string& seriesId)
  {
    loader_.ScheduleLoadSeries(seriesId);
  }


  void DicomSeriesVolumeSlicer::LoadInstance(const std::string& instanceId)
  {
    loader_.ScheduleLoadInstance(instanceId);
  }


  void DicomSeriesVolumeSlicer::LoadFrame(const std::string& instanceId,
                                          unsigned int frame)
  {
    loader_.ScheduleLoadFrame(instanceId, frame);
  }


  bool DicomSeriesVolumeSlicer::GetExtent(std::vector<OrthancStone::Vector>& points,
                                          const OrthancStone::CoordinateSystem3D& viewportSlice)
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

  
  void DicomSeriesVolumeSlicer::ScheduleLayerCreation(const OrthancStone::CoordinateSystem3D& viewportSlice)
  {
    size_t index;

    if (loader_.IsGeometryReady() &&
        loader_.LookupSlice(index, viewportSlice))
    {
      loader_.ScheduleLoadSliceImage(index, quality_);
    }
  }
}
