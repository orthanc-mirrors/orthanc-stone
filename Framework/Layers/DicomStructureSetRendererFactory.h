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

#include "LayerSourceBase.h"
#include "../Volumes/StructureSetLoader.h"

namespace OrthancStone
{
  class DicomStructureSetRendererFactory :
    public LayerSourceBase,
    private IVolumeLoader::IObserver
  {
  private:
    class Renderer;

    virtual void NotifyGeometryReady(const IVolumeLoader& loader)
    {
      LayerSourceBase::NotifyGeometryReady();
    }
      
    virtual void NotifyGeometryError(const IVolumeLoader& loader)
    {
      LayerSourceBase::NotifyGeometryError();
    }
      
    virtual void NotifyContentChange(const IVolumeLoader& loader)
    {
      LayerSourceBase::NotifyContentChange();
    }
    
    StructureSetLoader& loader_;

  public:
    DicomStructureSetRendererFactory(StructureSetLoader& loader) :
      loader_(loader)
    {
      loader_.Register(*this);
    }

    virtual bool GetExtent(std::vector<Vector>& points,
                           const CoordinateSystem3D& viewportSlice)
    {
      return false;
    }

    virtual void ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice);
  };
}
