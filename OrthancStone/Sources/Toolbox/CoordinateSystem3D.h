/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include "../Scene2D/ScenePoint2D.h"
#include "LinearAlgebra.h"
#include "OrthancDatasets/IDicomDataset.h"

#include <iosfwd>

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

    friend std::ostream& operator<< (std::ostream& s, const CoordinateSystem3D& that);

    CoordinateSystem3D(const Vector& origin,
                       const Vector& axisX,
                       const Vector& axisY);

    CoordinateSystem3D(const IDicomDataset& dicom);

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

    const Vector& GetOrigin() const  // This is the "Image Position Patient" tag
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

    void SetOrigin(const Vector& origin);

    Vector MapSliceToWorldCoordinates(double x,
                                      double y) const;
    
    Vector MapSliceToWorldCoordinates(const ScenePoint2D& p) const
    {
      return MapSliceToWorldCoordinates(p.GetX(), p.GetY());
    }
    
    double ProjectAlongNormal(const Vector& point) const;

    void ProjectPoint(double& offsetX,
                      double& offsetY,
                      const Vector& point) const;

    ScenePoint2D ProjectPoint(const Vector& point) const
    {
      double x, y;
      ProjectPoint(x, y, point);
      return ScenePoint2D(x, y);
    }

    /*
    Alternated faster implementation (untested yet)
    */
    void ProjectPoint2(double& offsetX, 
                       double& offsetY, 
                       const Vector& point) const;

    bool IntersectSegment(Vector& p,
                          const Vector& edgeFrom,
                          const Vector& edgeTo) const;

    bool IntersectLine(Vector& p,
                       const Vector& origin,
                       const Vector& direction) const;

    // Returns "false" is the two planes are not parallel
    static bool ComputeDistance(double& distance,
                                const CoordinateSystem3D& a,
                                const CoordinateSystem3D& b);

    // Normalize a cutting plane so that the origin (0,0,0) of the 3D
    // world is mapped to the origin of its (x,y) coordinate system
    static CoordinateSystem3D NormalizeCuttingPlane(const CoordinateSystem3D& plane);
  };
}