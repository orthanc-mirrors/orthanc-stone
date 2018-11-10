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


#include "VolumeSlicerBase.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  void VolumeSlicerBase::NotifyGeometryReady()
  {
    EmitMessage(IVolumeSlicer::GeometryReadyMessage(*this));
  }
    
  void VolumeSlicerBase::NotifyGeometryError()
  {
    EmitMessage(IVolumeSlicer::GeometryErrorMessage(*this));
  }
    
  void VolumeSlicerBase::NotifyContentChange()
  {
    EmitMessage(IVolumeSlicer::ContentChangedMessage(*this));
  }

  void VolumeSlicerBase::NotifySliceContentChange(const Slice& slice)
  {
    EmitMessage(IVolumeSlicer::SliceContentChangedMessage(*this, slice));
  }

  void VolumeSlicerBase::NotifyLayerReady(const LayerReadyMessage::IRendererFactory& factory,
                                          const CoordinateSystem3D& slice)
  {
    EmitMessage(IVolumeSlicer::LayerReadyMessage(*this, factory, slice));
  }

  void VolumeSlicerBase::NotifyLayerError(const CoordinateSystem3D& slice)
  {
    EmitMessage(IVolumeSlicer::LayerErrorMessage(*this, slice));
  }
}
