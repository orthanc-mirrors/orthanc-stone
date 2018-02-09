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

#include "GeometryToolbox.h"

#include <Plugins/Samples/Common/IDicomDataset.h>

namespace OrthancStone
{
  // Geometry of a 3D plane
  class CoordinateSystem3D
  {
  private:
    Vector    origin_;
    Vector    normal_;
    Vector    axisX_;
    Vector    axisY_;
    double    d_;

    void CheckAndComputeNormal();

    void Setup(const std::string& imagePositionPatient,
               const std::string& imageOrientationPatient);

    void SetupCanonical();

    double GetOffset() const;

  public:
    CoordinateSystem3D()
    {
      SetupCanonical();
    }

    CoordinateSystem3D(const Vector& origin,
                       const Vector& axisX,
                       const Vector& axisY);

    CoordinateSystem3D(const OrthancPlugins::IDicomDataset& dicom);

    CoordinateSystem3D(const std::string& imagePositionPatient,
                       const std::string& imageOrientationPatient)
    {
      Setup(imagePositionPatient, imageOrientationPatient);
    }

    CoordinateSystem3D(const Orthanc::DicomMap& dicom);

    const Vector& GetNormal() const
    {
      return normal_;
    }

    const Vector& GetOrigin() const
    {
      return origin_;
    }

    const Vector& GetAxisX() const
    {
      return axisX_;
    }

    const Vector& GetAxisY() const
    {
      return axisY_;
    }

    Vector MapSliceToWorldCoordinates(double x,
                                      double y) const;
    
    double ProjectAlongNormal(const Vector& point) const;

    void ProjectPoint(double& offsetX,
                      double& offsetY,
                      const Vector& point) const;

    bool IntersectSegment(Vector& p,
                          const Vector& edgeFrom,
                          const Vector& edgeTo) const;

    bool IntersectLine(Vector& p,
                       const Vector& origin,
                       const Vector& direction) const;
  };
}
