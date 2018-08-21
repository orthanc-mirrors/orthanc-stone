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
  void OrthancFrameLayerSource::HandleMessage(const IObservable& from, const IMessage& message)
  {
    switch (message.GetType())
    {
    case MessageType_SliceGeometryReady:
    {
      const OrthancSlicesLoader& loader = dynamic_cast<const OrthancSlicesLoader&>(from);
      if (loader.GetSliceCount() > 0)
      {
        LayerSourceBase::NotifyGeometryReady();
      }
      else
      {
        LayerSourceBase::NotifyGeometryError();
      }

    }; break;
    case MessageType_SliceGeometryError:
    {
      const OrthancSlicesLoader& loader = dynamic_cast<const OrthancSlicesLoader&>(from);
      LayerSourceBase::NotifyGeometryError();
    }; break;
    case MessageType_SliceImageReady:
    {
      const OrthancSlicesLoader::SliceImageReadyMessage& msg = dynamic_cast<const OrthancSlicesLoader::SliceImageReadyMessage&>(message);
      bool isFull = (msg.effectiveQuality_ == SliceImageQuality_FullPng || msg.effectiveQuality_ == SliceImageQuality_FullPam);
      LayerSourceBase::NotifyLayerReady(FrameRenderer::CreateRenderer(msg.image_.release(), msg.slice_, isFull),
                                        msg.slice_.GetGeometry(), false);

    }; break;
    case MessageType_SliceImageError:
    {
      const OrthancSlicesLoader::SliceImageErrorMessage& msg = dynamic_cast<const OrthancSlicesLoader::SliceImageErrorMessage&>(message);
      LayerSourceBase::NotifyLayerReady(NULL, msg.slice_.GetGeometry(), true);
    }; break;
    default:
      VLOG("unhandled message type" << message.GetType());
    }
  }


  OrthancFrameLayerSource::OrthancFrameLayerSource(MessageBroker& broker, IWebService& orthanc) :
    LayerSourceBase(broker),
    IObserver(broker),
    //OrthancSlicesLoader::ISliceLoaderObserver(broker),
    loader_(broker, orthanc),
    quality_(SliceImageQuality_FullPng)
  {
    loader_.RegisterObserver(*this);
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
