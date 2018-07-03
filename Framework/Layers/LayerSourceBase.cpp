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


#include "LayerSourceBase.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
//  namespace
//  {
//    class LayerReadyFunctor : public boost::noncopyable
//    {
//    private:
//      std::auto_ptr<ILayerRenderer>  layer_;
//      const CoordinateSystem3D&      slice_;
//      bool                           isError_;
      
//    public:
//      LayerReadyFunctor(ILayerRenderer* layer,
//                        const CoordinateSystem3D& slice,
//                        bool isError) :
//        layer_(layer),
//        slice_(slice),
//        isError_(isError)
//      {
//      }

//      void operator() (ILayerSource::IObserver& observer,
//                       const ILayerSource& source)
//      {
//        observer.NotifyLayerReady(layer_, source, slice_, isError_);
//      }
//    };
//  }

  void LayerSourceBase::NotifyGeometryReady()
  {
    Emit(IMessage(MessageType_GeometryReady));
  }
    
  void LayerSourceBase::NotifyGeometryError()
  {
    Emit(IMessage(MessageType_GeometryError));
  }
    
  void LayerSourceBase::NotifyContentChange()
  {
    Emit(IMessage(MessageType_ContentChanged));
  }

  void LayerSourceBase::NotifySliceChange(const Slice& slice)
  {
    Emit(ILayerSource::SliceChangedMessage(slice));
  }

  void LayerSourceBase::NotifyLayerReady(ILayerRenderer* layer,
                                         const CoordinateSystem3D& slice,
                                         bool isError)
  {
    std::auto_ptr<ILayerRenderer> renderer(layer);
    Emit(ILayerSource::LayerReadyMessage(renderer, slice, isError));
  }

}
