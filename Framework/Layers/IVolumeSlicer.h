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


#pragma once

#include "ILayerRenderer.h"
#include "../Toolbox/Slice.h"
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Messages/IMessage.h"
#include "Core/Images/Image.h"
#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class IVolumeSlicer : public IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, GeometryReadyMessage, IVolumeSlicer);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, GeometryErrorMessage, IVolumeSlicer);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, ContentChangedMessage, IVolumeSlicer);

    class SliceContentChangedMessage : public OriginMessage<IVolumeSlicer>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      const Slice& slice_;

    public:
      SliceContentChangedMessage(IVolumeSlicer& origin,
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
    

    class LayerReadyMessage : public OriginMessage<IVolumeSlicer>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    public:
      class IRendererFactory : public boost::noncopyable
      {
      public:
        virtual ~IRendererFactory()
        {
        }

        virtual ILayerRenderer* CreateRenderer() const = 0;
      };
    
    private:
      const IRendererFactory&    factory_;
      const CoordinateSystem3D&  slice_;

    public:
      LayerReadyMessage(IVolumeSlicer& origin,
                        const IRendererFactory& rendererFactory,
                        const CoordinateSystem3D& slice) :
        OriginMessage(origin),
        factory_(rendererFactory),
        slice_(slice)
      {
      }

      ILayerRenderer* CreateRenderer() const
      {
        return factory_.CreateRenderer();
      }

      const CoordinateSystem3D& GetSlice() const
      {
        return slice_;
      }
    };


    class LayerErrorMessage : public OriginMessage<IVolumeSlicer>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      const CoordinateSystem3D&  slice_;

    public:
      LayerErrorMessage(IVolumeSlicer& origin,
                        const CoordinateSystem3D& slice) :
        OriginMessage(origin),
        slice_(slice)
      {
      }

      const CoordinateSystem3D& GetSlice() const
      {
        return slice_;
      }
    };


    IVolumeSlicer(MessageBroker& broker) :
      IObservable(broker)
    {
    }
    
    virtual ~IVolumeSlicer()
    {
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice) = 0;

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice) = 0;
  };
}
