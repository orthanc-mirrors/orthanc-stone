/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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
#include "../../Messages/IObservable.h"
#include "../../Messages/IMessage.h"
#include "Core/Images/Image.h"
#include <boost/shared_ptr.hpp>

namespace Deprecated
{
  class IVolumeSlicer : public OrthancStone::IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, GeometryReadyMessage, IVolumeSlicer);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, GeometryErrorMessage, IVolumeSlicer);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, ContentChangedMessage, IVolumeSlicer);

    class SliceContentChangedMessage : public OrthancStone::OriginMessage<IVolumeSlicer>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      const Deprecated::Slice& slice_;

    public:
      SliceContentChangedMessage(IVolumeSlicer& origin,
                                 const Deprecated::Slice& slice) :
        OriginMessage(origin),
        slice_(slice)
      {
      }

      const Deprecated::Slice& GetSlice() const
      {
        return slice_;
      }
    };
    

    class LayerReadyMessage : public OrthancStone::OriginMessage<IVolumeSlicer>
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
      const OrthancStone::CoordinateSystem3D&  slice_;

    public:
      LayerReadyMessage(IVolumeSlicer& origin,
                        const IRendererFactory& rendererFactory,
                        const OrthancStone::CoordinateSystem3D& slice) :
        OriginMessage(origin),
        factory_(rendererFactory),
        slice_(slice)
      {
      }

      ILayerRenderer* CreateRenderer() const
      {
        return factory_.CreateRenderer();
      }

      const OrthancStone::CoordinateSystem3D& GetSlice() const
      {
        return slice_;
      }
    };


    class LayerErrorMessage : public OrthancStone::OriginMessage<IVolumeSlicer>
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      const OrthancStone::CoordinateSystem3D&  slice_;

    public:
      LayerErrorMessage(IVolumeSlicer& origin,
                        const OrthancStone::CoordinateSystem3D& slice) :
        OriginMessage(origin),
        slice_(slice)
      {
      }

      const OrthancStone::CoordinateSystem3D& GetSlice() const
      {
        return slice_;
      }
    };


    IVolumeSlicer(OrthancStone::MessageBroker& broker) :
      IObservable(broker)
    {
    }
    
    virtual ~IVolumeSlicer()
    {
    }

    virtual bool GetExtent(std::vector<OrthancStone::Vector>& points,
                           const OrthancStone::CoordinateSystem3D& viewportSlice) = 0;

    virtual void ScheduleLayerCreation(const OrthancStone::CoordinateSystem3D& viewportSlice) = 0;
  };
}
