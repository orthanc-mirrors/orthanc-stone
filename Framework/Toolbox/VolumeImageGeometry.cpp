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


#include "VolumeImageGeometry.h"

#include "../Toolbox/GeometryToolbox.h"

#include <Core/OrthancException.h>


namespace OrthancStone
{
  void VolumeImageGeometry::Invalidate()
  {
    Vector p = (axialGeometry_.GetOrigin() +
                static_cast<double>(depth_ - 1) * voxelDimensions_[2] * axialGeometry_.GetNormal());
        
    coronalGeometry_ = CoordinateSystem3D(p,
                                          axialGeometry_.GetAxisX(),
                                          -axialGeometry_.GetNormal());
    
    sagittalGeometry_ = CoordinateSystem3D(p,
                                           axialGeometry_.GetAxisY(),
                                           -axialGeometry_.GetNormal());

    Vector origin = (
      axialGeometry_.MapSliceToWorldCoordinates(-0.5 * voxelDimensions_[0],
                                                -0.5 * voxelDimensions_[1]) -
      0.5 * voxelDimensions_[2] * axialGeometry_.GetNormal());

    Vector scaling;
    
    if (width_ == 0 ||
        height_ == 0 ||
        depth_ == 0)
    {
      LinearAlgebra::AssignVector(scaling, 1, 1, 1);
    }
    else
    {
      scaling = (
        axialGeometry_.GetAxisX() * voxelDimensions_[0] * static_cast<double>(width_) +
        axialGeometry_.GetAxisY() * voxelDimensions_[1] * static_cast<double>(height_) +
        axialGeometry_.GetNormal() * voxelDimensions_[2] * static_cast<double>(depth_));
    }

    transform_ = LinearAlgebra::Product(
      GeometryToolbox::CreateTranslationMatrix(origin[0], origin[1], origin[2]),
      GeometryToolbox::CreateScalingMatrix(scaling[0], scaling[1], scaling[2]));

    LinearAlgebra::InvertMatrix(transformInverse_, transform_);
  }

  
  VolumeImageGeometry::VolumeImageGeometry() :
    width_(0),
    height_(0),
    depth_(0)
  {
    LinearAlgebra::AssignVector(voxelDimensions_, 1, 1, 1);
    Invalidate();
  }


  void VolumeImageGeometry::SetSize(unsigned int width,
                                    unsigned int height,
                                    unsigned int depth)
  {
    width_ = width;
    height_ = height;
    depth_ = depth;
    Invalidate();
  }

  
  void VolumeImageGeometry::SetAxialGeometry(const CoordinateSystem3D& geometry)
  {
    axialGeometry_ = geometry;
    Invalidate();
  }


  void VolumeImageGeometry::SetVoxelDimensions(double x,
                                               double y,
                                               double z)
  {
    if (x <= 0 ||
        y <= 0 ||
        z <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      LinearAlgebra::AssignVector(voxelDimensions_, x, y, z);
      Invalidate();
    }
  }


  Vector VolumeImageGeometry::GetVoxelDimensions(VolumeProjection projection) const
  {
    switch (projection)
    {
      case VolumeProjection_Axial:
        return voxelDimensions_;

      case VolumeProjection_Coronal:
        return LinearAlgebra::CreateVector(voxelDimensions_[0], voxelDimensions_[2], voxelDimensions_[1]);

      case VolumeProjection_Sagittal:
        return LinearAlgebra::CreateVector(voxelDimensions_[1], voxelDimensions_[2], voxelDimensions_[0]);

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  void VolumeImageGeometry::GetSliceSize(unsigned int& width,
                                         unsigned int& height,
                                         VolumeProjection projection)
  {
    switch (projection)
    {
      case VolumeProjection_Axial:
        width = width_;
        height = height_;
        break;

      case VolumeProjection_Coronal:
        width = width_;
        height = depth_;
        break;

      case VolumeProjection_Sagittal:
        width = height_;
        height = depth_;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }



  Vector VolumeImageGeometry::GetCoordinates(float x,
                                             float y,
                                             float z) const
  {
    Vector p = LinearAlgebra::Product(transform_, LinearAlgebra::CreateVector(x, y, z, 1));

    assert(LinearAlgebra::IsNear(p[3], 1));  // Affine transform, no perspective effect

    // Back to non-homogeneous coordinates
    return LinearAlgebra::CreateVector(p[0], p[1], p[2]);
  }


  bool VolumeImageGeometry::DetectProjection(VolumeProjection& projection,
                                             const CoordinateSystem3D& plane) const
  {
    if (GeometryToolbox::IsParallel(plane.GetNormal(),
                                    axialGeometry_.GetNormal()))
    {
      projection = VolumeProjection_Axial;
      return true;
    }
    else if (GeometryToolbox::IsParallel(plane.GetNormal(),
                                         coronalGeometry_.GetNormal()))
    {
      projection = VolumeProjection_Coronal;
      return true;
    }
    else if (GeometryToolbox::IsParallel(plane.GetNormal(),
                                         sagittalGeometry_.GetNormal()))
    {
      projection = VolumeProjection_Sagittal;
      return true;
    }
    else
    {
      return false;
    }
  }
}
