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
#include "../Toolbox/SliceGeometry.h"
#include "../Volumes/ISliceableVolume.h"

namespace OrthancStone
{
  class ILayerRendererFactory : public IThreadUnsafe
  {
  public:
    virtual ~ILayerRendererFactory()
    {
    }

    virtual bool GetExtent(double& x1,
                           double& y1,
                           double& x2,
                           double& y2,
                           const SliceGeometry& displaySlice) = 0;

    // This operation can be slow, as it might imply the download of a
    // slice from Orthanc. The result might be NULL, if the slice is
    // not compatible with the underlying source volume.
    virtual ILayerRenderer* CreateLayerRenderer(const SliceGeometry& displaySlice) = 0;

    virtual bool HasSourceVolume() const = 0;

    virtual ISliceableVolume& GetSourceVolume() const = 0;
  };
}
