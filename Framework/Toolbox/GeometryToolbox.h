/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

// Patch for Boost 1.64.0
// https://github.com/dealii/dealii/issues/4302
#if BOOST_VERSION >= 106300  // or 64, need to check
#  include <boost/serialization/array_wrapper.hpp>
#endif

#include <boost/numeric/ublas/vector.hpp>

#include "../../Resources/Orthanc/Plugins/Samples/Common/DicomDatasetReader.h"

namespace OrthancStone
{
  typedef boost::numeric::ublas::vector<double>   Vector;

  namespace GeometryToolbox
  {
    void Print(const Vector& v);

    bool ParseVector(Vector& target,
                     const std::string& s);

    bool ParseVector(Vector& target,
                     const OrthancPlugins::IDicomDataset& dataset,
                     const OrthancPlugins::DicomPath& tag);

    void AssignVector(Vector& v,
                      double v1,
                      double v2);

    void AssignVector(Vector& v,
                      double v1,
                      double v2,
                      double v3);

    inline bool IsNear(double x,
                       double y,
                       double threshold)
    {
      return fabs(x - y) < threshold;
    }

    bool IsNear(double x,
                double y);

    inline bool IsCloseToZero(double x)
    {
      return IsNear(x, 0.0);
    }

    void NormalizeVector(Vector& u);

    void CrossProduct(Vector& result,
                      const Vector& u,
                      const Vector& v);

    void ProjectPointOntoPlane(Vector& result,
                               const Vector& point,
                               const Vector& planeNormal,
                               const Vector& planeOrigin);

    bool IsParallel(const Vector& u,
                    const Vector& v);

    bool IsParallelOrOpposite(bool& isOpposite,
                              const Vector& u,
                              const Vector& v);

    bool IntersectTwoPlanes(Vector& p,
                            Vector& direction,
                            const Vector& origin1,
                            const Vector& normal1,
                            const Vector& origin2,
                            const Vector& normal2);

    bool ClipLineToRectangle(double& x1,  // Coordinates of the clipped line (out)
                             double& y1,
                             double& x2,
                             double& y2,
                             const double ax,  // Two points defining the line (in)
                             const double ay,
                             const double bx,
                             const double by,
                             const double& xmin,   // Coordinates of the rectangle (in)
                             const double& ymin,
                             const double& xmax,
                             const double& ymax);

    void GetPixelSpacing(double& spacingX, 
                         double& spacingY,
                         const OrthancPlugins::IDicomDataset& dicom);
  };
}
