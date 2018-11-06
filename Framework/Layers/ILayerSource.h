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
#include "Core/Images/Image.h"
#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class ILayerSource : public IObservable
  {
  public:
    typedef OriginMessage<MessageType_LayerSource_GeometryReady, ILayerSource>  GeometryReadyMessage;
    typedef OriginMessage<MessageType_LayerSource_GeometryError, ILayerSource>  GeometryErrorMessage;
    typedef OriginMessage<MessageType_LayerSource_ContentChanged, ILayerSource> ContentChangedMessage;

    class SliceChangedMessage : public OriginMessage<MessageType_LayerSource_SliceChanged, ILayerSource>
    {
    private:
      const Slice& slice_;

    public:
      SliceChangedMessage(ILayerSource& origin,
                          const Slice& slice) :
        OriginMessage(origin),
        slice_(slice)
      {
      }

      const Slice& GetSlice() const
      {
        return slice_;
      }
    };
    

    class LayerReadyMessage : public OriginMessage<MessageType_LayerSource_LayerReady, ILayerSource>
    {
    private:
      std::auto_ptr<ILayerRenderer> renderer_;
      const CoordinateSystem3D& slice_;
      bool isError_;

    public:
      LayerReadyMessage(ILayerSource& origin,
                        ILayerRenderer* renderer,  // Takes ownership => TODO Remove this!
                        const CoordinateSystem3D& slice,
                        bool isError  // TODO => create NotifyLayerError
                        ) :
        OriginMessage(origin),
        renderer_(renderer),
        slice_(slice),
        isError_(isError)
      {
      }

      // TODO - Remove this function
      std::auto_ptr<ILayerRenderer>& GetRendererRaw()
      {
        return renderer_;
      }

      const ILayerRenderer& GetRenderer() const
      {
        return *renderer_;
      }

      const CoordinateSystem3D& GetSlice() const
      {
        return slice_;
      }

      bool IsError() const
      {
        return isError_;
      }
    };


    // TODO: Rename "ImageReadyMessage" as "SliceReadyMessage"
    class ImageReadyMessage : public OriginMessage<MessageType_LayerSource_ImageReady, ILayerSource>
    {
    private:
      const Orthanc::ImageAccessor&  image_;
      SliceImageQuality              imageQuality_;
      const Slice&                   slice_;

    public:
      ImageReadyMessage(ILayerSource& origin,
                        const Orthanc::ImageAccessor& image,
                        SliceImageQuality imageQuality,
                        const Slice& slice) :
        OriginMessage(origin),
        image_(image),
        imageQuality_(imageQuality),
        slice_(slice)
      {
      }

      const Orthanc::ImageAccessor& GetImage() const
      {
        return image_;
      }

      SliceImageQuality GetImageQuality() const
      {
        return imageQuality_;
      }

      const Slice& GetSlice() const
      {
        return slice_;
      }
    };

    
    ILayerSource(MessageBroker& broker) :
      IObservable(broker)
    {
    }
    
    virtual ~ILayerSource()
    {
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice) = 0;

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice) = 0;
  };
}
