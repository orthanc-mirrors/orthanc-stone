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

#include "../StoneEnumerations.h"
#include "CoordinateSystem3D.h"

namespace OrthancStone
{
  class VolumeImageGeometry
  {
  private:
    unsigned int           width_;
    unsigned int           height_;
    unsigned int           depth_;
    CoordinateSystem3D     axialGeometry_;
    CoordinateSystem3D     coronalGeometry_;
    CoordinateSystem3D     sagittalGeometry_;
    Vector                 voxelDimensions_;
    Matrix                 transform_;
    Matrix                 transformInverse_;

    void Invalidate();

  public:
    VolumeImageGeometry();

    unsigned int GetWidth() const
    {
      return width_;
    }

    unsigned int GetHeight() const
    {
      return height_;
    }

    unsigned int GetDepth() const
    {
      return depth_;
    }

    const CoordinateSystem3D& GetAxialGeometry() const
    {
      return axialGeometry_;
    }

    const CoordinateSystem3D& GetCoronalGeometry() const
    {
      return coronalGeometry_;
    }

    const CoordinateSystem3D& GetSagittalGeometry() const
    {
      return sagittalGeometry_;
    }

    void SetSize(unsigned int width,
                 unsigned int height,
                 unsigned int depth);

    // Set the geometry of the first axial slice (i.e. the one whose
    // depth == 0)
    void SetAxialGeometry(const CoordinateSystem3D& geometry);

    void SetVoxelDimensions(double x,
                            double y,
                            double z);

    Vector GetVoxelDimensions(VolumeProjection projection) const;

    void GetSliceSize(unsigned int& width,
                      unsigned int& height,
                      VolumeProjection projection);
    
    // Get the 3D position of a point in the volume, where x, y and z
    // lie in the [0;1] range
    Vector GetCoordinates(float x,
                          float y,
                          float z) const;

    bool DetectProjection(VolumeProjection& projection,
                          const CoordinateSystem3D& plane) const;
  };
}
