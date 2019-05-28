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


#include "ParallelSlices.h"

#include "../../Toolbox/GeometryToolbox.h"
#include "../../Volumes/ImageBuffer3D.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace Deprecated
{
  ParallelSlices::ParallelSlices()
  {
    Clear();
  }


  ParallelSlices::ParallelSlices(const ParallelSlices& other)
  {
    normal_ = other.normal_;

    slices_.resize(other.slices_.size());

    for (size_t i = 0; i < slices_.size(); i++)
    {
      assert(other.slices_[i] != NULL);
      slices_[i] = new OrthancStone::CoordinateSystem3D(*other.slices_[i]);
    }
  }


  void ParallelSlices::Clear()
  {
    for (size_t i = 0; i < slices_.size(); i++)
    {
      if (slices_[i] != NULL)
      {
        delete slices_[i];
        slices_[i] = NULL;
      }
    }

    slices_.clear();
    OrthancStone::LinearAlgebra::AssignVector(normal_, 0, 0, 1);
  }
  

  ParallelSlices::~ParallelSlices()
  {
    Clear();
  }


  void ParallelSlices::AddSlice(const OrthancStone::CoordinateSystem3D& slice)
  {
    if (slices_.empty())
    {
      normal_ = slice.GetNormal();
      slices_.push_back(new OrthancStone::CoordinateSystem3D(slice));
    }
    else if (OrthancStone::GeometryToolbox::IsParallel(slice.GetNormal(), normal_))
    {
      slices_.push_back(new OrthancStone::CoordinateSystem3D(slice));
    }
    else
    {
      LOG(ERROR) << "Trying to add a slice that is not parallel to the previous ones";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  void ParallelSlices::AddSlice(const OrthancStone::Vector& origin,
                                const OrthancStone::Vector& axisX,
                                const OrthancStone::Vector& axisY)
  {
    OrthancStone::CoordinateSystem3D slice(origin, axisX, axisY);
    AddSlice(slice);
  }


  const OrthancStone::CoordinateSystem3D& ParallelSlices::GetSlice(size_t index) const
  {
    if (index >= slices_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return *slices_[index];
    }
  }


  bool ParallelSlices::ComputeClosestSlice(size_t& closestSlice,
                                           double& closestDistance,
                                           const OrthancStone::Vector& origin) const
  {
    if (slices_.empty())
    {
      return false;
    }

    double reference = boost::numeric::ublas::inner_prod(origin, normal_);

    closestSlice = 0;
    closestDistance = std::numeric_limits<double>::infinity();

    for (size_t i = 0; i < slices_.size(); i++)
    {
      double distance = fabs(boost::numeric::ublas::inner_prod(slices_[i]->GetOrigin(), normal_) - reference);

      if (distance < closestDistance)
      {
        closestSlice = i;
        closestDistance = distance;
      }
    }

    return true;
  }


  ParallelSlices* ParallelSlices::Reverse() const
  {
    std::auto_ptr<ParallelSlices> reversed(new ParallelSlices);

    for (size_t i = slices_.size(); i > 0; i--)
    {
      const OrthancStone::CoordinateSystem3D& slice = *slices_[i - 1];

      reversed->AddSlice(slice.GetOrigin(),
                         -slice.GetAxisX(),
                         slice.GetAxisY());
    }

    return reversed.release();
  }


  ParallelSlices* ParallelSlices::FromVolumeImage(const OrthancStone::VolumeImageGeometry& geometry,
                                                  OrthancStone::VolumeProjection projection)
  {
    const OrthancStone::Vector dimensions = geometry.GetVoxelDimensions(OrthancStone::VolumeProjection_Axial);
    const OrthancStone::CoordinateSystem3D& axial = geometry.GetAxialGeometry();
    
    std::auto_ptr<ParallelSlices> result(new ParallelSlices);

    switch (projection)
    {
      case OrthancStone::VolumeProjection_Axial:
        for (unsigned int z = 0; z < geometry.GetDepth(); z++)
        {
          OrthancStone::Vector origin = axial.GetOrigin();
          origin += static_cast<double>(z) * dimensions[2] * axial.GetNormal();

          result->AddSlice(origin,
                           axial.GetAxisX(),
                           axial.GetAxisY());
        }
        break;

      case OrthancStone::VolumeProjection_Coronal:
        for (unsigned int y = 0; y < geometry.GetHeight(); y++)
        {
          OrthancStone::Vector origin = axial.GetOrigin();
          origin += static_cast<double>(y) * dimensions[1] * axial.GetAxisY();
          origin += static_cast<double>(geometry.GetDepth() - 1) * dimensions[2] * axial.GetNormal();

          result->AddSlice(origin,
                           axial.GetAxisX(),
                           -axial.GetNormal());
        }
        break;

      case OrthancStone::VolumeProjection_Sagittal:
        for (unsigned int x = 0; x < geometry.GetWidth(); x++)
        {
          OrthancStone::Vector origin = axial.GetOrigin();
          origin += static_cast<double>(x) * dimensions[0] * axial.GetAxisX();
          origin += static_cast<double>(geometry.GetDepth() - 1) * dimensions[2] * axial.GetNormal();

          result->AddSlice(origin,
                           axial.GetAxisY(),
                           -axial.GetNormal());
        }
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    return result.release();
  }
}
