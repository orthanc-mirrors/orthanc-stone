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
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Messages/IMessage.h"

namespace OrthancStone
{
  class ILayerSource : public IObservable
  {
  public:

    typedef OriginMessage<MessageType_LayerSource_GeometryReady, ILayerSource> GeometryReadyMessage;
    typedef OriginMessage<MessageType_LayerSource_GeometryError, ILayerSource> GeometryErrorMessage;
    typedef OriginMessage<MessageType_LayerSource_ContentChanged, ILayerSource> ContentChangedMessage;

    struct SliceChangedMessage : public OriginMessage<MessageType_LayerSource_SliceChanged, ILayerSource>
    {
      const Slice& slice_;
      SliceChangedMessage(ILayerSource& origin, const Slice& slice)
        : OriginMessage(origin),
          slice_(slice)
      {
      }
    };

    struct LayerReadyMessage : public OriginMessage<MessageType_LayerSource_LayerReady,ILayerSource>
    {
      std::auto_ptr<ILayerRenderer>& renderer_;
      const CoordinateSystem3D& slice_;
      bool isError_;

      LayerReadyMessage(ILayerSource& origin,
                        std::auto_ptr<ILayerRenderer>& layer,
                        const CoordinateSystem3D& slice,
                        bool isError  // TODO Shouldn't this be separate as NotifyLayerError?
                        )
        : OriginMessage(origin),
          renderer_(layer),
          slice_(slice),
          isError_(isError)
      {
      }
    };
    
    ILayerSource(MessageBroker& broker)
      : IObservable(broker)
    {}

    virtual ~ILayerSource()
    {
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice) = 0;

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice) = 0;
  };
}
