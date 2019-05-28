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

#include "../../Toolbox/CoordinateSystem3D.h"
#include "../../Volumes/VolumeImageGeometry.h"

namespace Deprecated
{
  class ParallelSlices : public boost::noncopyable
  {
  private:
    OrthancStone::Vector  normal_;
    std::vector<OrthancStone::CoordinateSystem3D*>  slices_;
    
    ParallelSlices& operator= (const ParallelSlices& other);  // Forbidden

    void Clear();

  public:
    ParallelSlices();

    ParallelSlices(const ParallelSlices& other);

    ~ParallelSlices();

    const OrthancStone::Vector& GetNormal() const
    {
      return normal_;
    }

    void AddSlice(const OrthancStone::CoordinateSystem3D& slice);

    void AddSlice(const OrthancStone::Vector& origin,
                  const OrthancStone::Vector& axisX,
                  const OrthancStone::Vector& axisY);

    size_t GetSliceCount() const
    {
      return slices_.size();
    }

    const OrthancStone::CoordinateSystem3D& GetSlice(size_t index) const;

    bool ComputeClosestSlice(size_t& closestSlice,
                             double& closestDistance,
                             const OrthancStone::Vector& origin) const;

    ParallelSlices* Reverse() const;

    static ParallelSlices* FromVolumeImage(const OrthancStone::VolumeImageGeometry& geometry,
                                           OrthancStone::VolumeProjection projection);
  };
}
