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
  void LayerSourceBase::NotifyGeometryReady()
  {
    EmitMessage(IMessage(MessageType_LayerSource_GeometryReady));
  }
    
  void LayerSourceBase::NotifyGeometryError()
  {
    EmitMessage(IMessage(MessageType_LayerSource_GeometryError));
  }
    
  void LayerSourceBase::NotifyContentChange()
  {
    EmitMessage(IMessage(MessageType_LayerSource_ContentChanged));
  }

  void LayerSourceBase::NotifySliceChange(const Slice& slice)
  {
    EmitMessage(ILayerSource::SliceChangedMessage(slice));
  }

  void LayerSourceBase::NotifyLayerReady(ILayerRenderer* layer,
                                         const CoordinateSystem3D& slice,
                                         bool isError)
  {
    std::auto_ptr<ILayerRenderer> renderer(layer);
    EmitMessage(ILayerSource::LayerReadyMessage(renderer, slice, isError));
  }

}
