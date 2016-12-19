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


#pragma once

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
