/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "ParallelSlices.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  ParallelSlices::ParallelSlices()
  {
    GeometryToolbox::AssignVector(normal_, 0, 0, 1);
  }


  ParallelSlices::ParallelSlices(const ParallelSlices& other)
  {
    normal_ = other.normal_;

    slices_.resize(other.slices_.size());

    for (size_t i = 0; i < slices_.size(); i++)
    {
      assert(other.slices_[i] != NULL);
      slices_[i] = new SliceGeometry(*other.slices_[i]);
    }
  }


  ParallelSlices::~ParallelSlices()
  {
    for (size_t i = 0; i < slices_.size(); i++)
    {
      if (slices_[i] != NULL)
      {
        delete slices_[i];
        slices_[i] = NULL;
      }
    }
  }


  void ParallelSlices::AddSlice(const SliceGeometry& slice)
  {
    if (slices_.empty())
    {
      normal_ = slice.GetNormal();
      slices_.push_back(new SliceGeometry(slice));
    }
    else if (GeometryToolbox::IsParallel(slice.GetNormal(), normal_))
    {
      slices_.push_back(new SliceGeometry(slice));
    }
    else
    {
      LOG(ERROR) << "Trying to add a slice that is not parallel to the previous ones";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  void ParallelSlices::AddSlice(const Vector& origin,
                                const Vector& axisX,
                                const Vector& axisY)
  {
    SliceGeometry slice(origin, axisX, axisY);
    AddSlice(slice);
  }


  const SliceGeometry& ParallelSlices::GetSlice(size_t index) const
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
                                           const Vector& origin) const
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
      const SliceGeometry& slice = *slices_[i - 1];

      reversed->AddSlice(slice.GetOrigin(),
                         -slice.GetAxisX(),
                         slice.GetAxisY());
    }

    return reversed.release();
  }
}
