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


#include "GeometryToolbox.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Core/Toolbox.h"

#include <stdio.h>
#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  namespace GeometryToolbox
  {
    void Print(const Vector& v)
    {
      for (size_t i = 0; i < v.size(); i++)
      {
        printf("%8.2f\n", v[i]);
      }
      printf("\n");
    }


    bool ParseVector(Vector& target,
                     const std::string& value)
    {
      std::vector<std::string> items;
      Orthanc::Toolbox::TokenizeString(items, value, '\\');

      target.resize(items.size());

      for (size_t i = 0; i < items.size(); i++)
      {
        try
        {
          target[i] = boost::lexical_cast<double>(Orthanc::Toolbox::StripSpaces(items[i]));
        }
        catch (boost::bad_lexical_cast&)
        {
          target.clear();
          return false;
        }
      }

      return true;
    }


    bool ParseVector(Vector& target,
                     const OrthancPlugins::IDicomDataset& dataset,
                     const OrthancPlugins::DicomPath& tag)
    {
      std::string value;
      return (dataset.GetStringValue(value, tag) &&
              ParseVector(target, value));
    }


    void AssignVector(Vector& v,
                      double v1,
                      double v2)
    {
      v.resize(2);
      v[0] = v1;
      v[1] = v2;
    }


    void AssignVector(Vector& v,
                      double v1,
                      double v2,
                      double v3)
    {
      v.resize(3);
      v[0] = v1;
      v[1] = v2;
      v[2] = v3;
    }


    bool IsNear(double x,
                double y)
    {
      // As most input is read as single-precision numbers, we take the
      // epsilon machine for float32 into consideration to compare numbers
      return IsNear(x, y, 10.0 * std::numeric_limits<float>::epsilon());
    }


    void NormalizeVector(Vector& u)
    {
      double norm = boost::numeric::ublas::norm_2(u);
      if (!IsCloseToZero(norm))
      {
        u = u / norm;
      }
    }


    void CrossProduct(Vector& result,
                      const Vector& u,
                      const Vector& v)
    {
      if (u.size() != 3 ||
          v.size() != 3)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      result.resize(3);

      result[0] = u[1] * v[2] - u[2] * v[1];
      result[1] = u[2] * v[0] - u[0] * v[2];
      result[2] = u[0] * v[1] - u[1] * v[0];
    }


    void ProjectPointOntoPlane(Vector& result,
                               const Vector& point,
                               const Vector& planeNormal,
                               const Vector& planeOrigin)
    {
      double norm =  boost::numeric::ublas::norm_2(planeNormal);
      if (IsCloseToZero(norm))
      {
        // Division by zero
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      // Make sure the norm of the normal is 1
      Vector n;
      n = planeNormal / norm;

      // Algebraic form of line–plane intersection, where the line passes
      // through "point" along the direction "normal" (thus, l == n)
      // https://en.wikipedia.org/wiki/Line%E2%80%93plane_intersection#Algebraic_form
      result = boost::numeric::ublas::inner_prod(planeOrigin - point, n) * n + point;
    }



    bool IsParallelOrOpposite(bool& isOpposite,
                              const Vector& u,
                              const Vector& v)
    {
      // The dot product of the two vectors gives the cosine of the angle
      // between the vectors
      // https://en.wikipedia.org/wiki/Dot_product

      double normU = boost::numeric::ublas::norm_2(u);
      double normV = boost::numeric::ublas::norm_2(v);

      if (IsCloseToZero(normU) ||
          IsCloseToZero(normV))
      {
        return false;
      }

      double cosAngle = boost::numeric::ublas::inner_prod(u, v) / (normU * normV);

      // The angle must be zero, so the cosine must be almost equal to
      // cos(0) == 1 (or cos(180) == -1 if allowOppositeDirection == true)

      if (IsCloseToZero(cosAngle - 1.0))
      {
        isOpposite = false;
        return true;
      }
      else if (IsCloseToZero(fabs(cosAngle) - 1.0))
      {
        isOpposite = true;
        return true;
      }
      else
      {
        return false;
      }
    }


    bool IsParallel(const Vector& u,
                    const Vector& v)
    {
      bool isOpposite;
      return (IsParallelOrOpposite(isOpposite, u, v) &&
              !isOpposite);
    }


    bool IntersectTwoPlanes(Vector& p,
                            Vector& direction,
                            const Vector& origin1,
                            const Vector& normal1,
                            const Vector& origin2,
                            const Vector& normal2)
    {
      // This is "Intersection of 2 Planes", possibility "(C) 3 Plane
      // Intersect Point" of:
      // http://geomalgorithms.com/a05-_intersect-1.html

      // The direction of the line of intersection is orthogonal to the
      // normal of both planes
      CrossProduct(direction, normal1, normal2);

      double norm = boost::numeric::ublas::norm_2(direction);
      if (IsCloseToZero(norm))
      {
        // The two planes are parallel or coincident
        return false;
      }

      double d1 = -boost::numeric::ublas::inner_prod(normal1, origin1);
      double d2 = -boost::numeric::ublas::inner_prod(normal2, origin2);
      Vector tmp = d2 * normal1 - d1 * normal2;

      CrossProduct(p, tmp, direction);
      p /= norm;

      return true;
    }


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
                             const double& ymax)
    {
      // This is Skala algorithm for rectangles, "A new approach to line
      // and line segment clipping in homogeneous coordinates"
      // (2005). This is a direct, non-optimized translation of Algorithm
      // 2 in the paper.

      static uint8_t tab1[16] = { 255 /* none */,
                                  0,
                                  0,
                                  1,
                                  1,
                                  255 /* na */,
                                  0,
                                  2,
                                  2,
                                  0,
                                  255 /* na */,
                                  1,
                                  1,
                                  0,
                                  0,
                                  255 /* none */ };


      static uint8_t tab2[16] = { 255 /* none */,
                                  3,
                                  1,
                                  3,
                                  2,
                                  255 /* na */,
                                  2,
                                  3,
                                  3,
                                  2,
                                  255 /* na */,
                                  2,
                                  3,
                                  1,
                                  3,
                                  255 /* none */ };

      // Create the coordinates of the rectangle
      Vector x[4];
      AssignVector(x[0], xmin, ymin, 1.0);
      AssignVector(x[1], xmax, ymin, 1.0);
      AssignVector(x[2], xmax, ymax, 1.0);
      AssignVector(x[3], xmin, ymax, 1.0);

      // Move to homogoneous coordinates in 2D
      Vector p;

      {
        Vector a, b;
        AssignVector(a, ax, ay, 1.0);
        AssignVector(b, bx, by, 1.0);
        CrossProduct(p, a, b);
      }

      uint8_t c = 0;

      for (unsigned int k = 0; k < 4; k++)
      {
        if (boost::numeric::ublas::inner_prod(p, x[k]) >= 0)
        {
          c |= (1 << k);
        }
      }

      assert(c < 16);

      uint8_t i = tab1[c];
      uint8_t j = tab2[c];

      if (i == 255 || j == 255)
      {
        return false;   // No intersection
      }
      else
      {
        Vector a, b, e;
        CrossProduct(e, x[i], x[(i + 1) % 4]);
        CrossProduct(a, p, e);
        CrossProduct(e, x[j], x[(j + 1) % 4]);
        CrossProduct(b, p, e);

        // Go back to non-homogeneous coordinates
        x1 = a[0] / a[2];
        y1 = a[1] / a[2];
        x2 = b[0] / b[2];
        y2 = b[1] / b[2];

        return true;
      }
    }


    void GetPixelSpacing(double& spacingX, 
                         double& spacingY,
                         const OrthancPlugins::IDicomDataset& dicom)
    {
      Vector v;

      if (ParseVector(v, dicom, OrthancPlugins::DICOM_TAG_PIXEL_SPACING))
      {
        if (v.size() != 2 ||
            v[0] <= 0 ||
            v[1] <= 0)
        {
          LOG(ERROR) << "Bad value for PixelSpacing tag";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          spacingX = v[0];
          spacingY = v[1];
        }
      }
      else
      {
        // The "PixelSpacing" is of type 1C: It could be absent, use
        // default value in such a case
        spacingX = 1;
        spacingY = 1;
      }
    }
  }
}
