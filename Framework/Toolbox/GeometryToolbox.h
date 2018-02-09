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

// Patch for ublas in Boost 1.64.0
// https://github.com/dealii/dealii/issues/4302
#include <boost/version.hpp>
#if BOOST_VERSION >= 106300  // or 64, need to check
#  include <boost/serialization/array_wrapper.hpp>
#endif

#include <Core/DicomFormat/DicomMap.h>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <cassert>

namespace OrthancStone
{
  typedef boost::numeric::ublas::matrix<double>   Matrix;
  typedef boost::numeric::ublas::vector<double>   Vector;

  namespace GeometryToolbox
  {
    void Print(const Vector& v);

    void Print(const Matrix& m);

    bool ParseVector(Vector& target,
                     const std::string& s);

    bool ParseVector(Vector& target,
                     const Orthanc::DicomMap& dataset,
                     const Orthanc::DicomTag& tag);

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
                         const Orthanc::DicomMap& dicom);

    inline double ProjectAlongNormal(const Vector& point,
                                     const Vector& normal)
    {
      return boost::numeric::ublas::inner_prod(point, normal);
    }

    Matrix CreateRotationMatrixAlongX(double a);

    Matrix CreateRotationMatrixAlongY(double a);

    Matrix CreateRotationMatrixAlongZ(double a);

    bool IntersectPlaneAndSegment(Vector& p,
                                  const Vector& normal,
                                  double d,
                                  const Vector& edgeFrom,
                                  const Vector& edgeTo);

    bool IntersectPlaneAndLine(Vector& p,
                               const Vector& normal,
                               double d,
                               const Vector& origin,
                               const Vector& direction);

    void FillMatrix(Matrix& target,
                    size_t rows,
                    size_t columns,
                    const double values[]);

    void FillVector(Vector& target,
                    size_t size,
                    const double values[]);

    void Convert(Matrix& target,
                 const Vector& source);

    inline float ComputeBilinearInterpolationInternal(float x,
                                                      float y,
                                                      float f00,    // source(x, y)
                                                      float f01,    // source(x + 1, y)
                                                      float f10,    // source(x, y + 1)
                                                      float f11);   // source(x + 1, y + 1)

    inline float ComputeBilinearInterpolation(float x,
                                              float y,
                                              float f00,    // source(x, y)
                                              float f01,    // source(x + 1, y)
                                              float f10,    // source(x, y + 1)
                                              float f11);   // source(x + 1, y + 1)

    inline float ComputeTrilinearInterpolation(float x,
                                               float y,
                                               float z,
                                               float f000,   // source(x, y, z)
                                               float f001,   // source(x + 1, y, z)
                                               float f010,   // source(x, y + 1, z)
                                               float f011,   // source(x + 1, y + 1, z)
                                               float f100,   // source(x, y, z + 1)
                                               float f101,   // source(x + 1, y, z + 1)
                                               float f110,   // source(x, y + 1, z + 1)
                                               float f111);  // source(x + 1, y + 1, z + 1)
  };
}


float OrthancStone::GeometryToolbox::ComputeBilinearInterpolationInternal(float x,
                                                                          float y,
                                                                          float f00,
                                                                          float f01,
                                                                          float f10,
                                                                          float f11)
{
  // This function works on fractional parts
  assert(x >= 0 && y >= 0 && x < 1 && y < 1);

  // https://en.wikipedia.org/wiki/Bilinear_interpolation#Unit_square
  return f00 * (1 - x) * (1 - y) + f01 * x * (1 - y) + f10 * (1 - x) * y + f11 * x * y;
}


float OrthancStone::GeometryToolbox::ComputeBilinearInterpolation(float x,
                                                                  float y,
                                                                  float f00,
                                                                  float f01,
                                                                  float f10,
                                                                  float f11)
{
  assert(x >= 0 && y >= 0);

  // Compute the fractional part of (x,y) (this is the "floor()"
  // operation on positive integers)
  float xx = x - static_cast<float>(static_cast<int>(x));
  float yy = y - static_cast<float>(static_cast<int>(y));

  return ComputeBilinearInterpolationInternal(xx, yy, f00, f01, f10, f11);
}


float OrthancStone::GeometryToolbox::ComputeTrilinearInterpolation(float x,
                                                                   float y,
                                                                   float z,
                                                                   float f000,
                                                                   float f001,
                                                                   float f010,
                                                                   float f011,
                                                                   float f100,
                                                                   float f101,
                                                                   float f110,
                                                                   float f111)
{
  assert(x >= 0 && y >= 0 && z >= 0);

  float xx = x - static_cast<float>(static_cast<int>(x));
  float yy = y - static_cast<float>(static_cast<int>(y));
  float zz = z - static_cast<float>(static_cast<int>(z));

  // "In practice, a trilinear interpolation is identical to two
  // bilinear interpolation combined with a linear interpolation"
  // https://en.wikipedia.org/wiki/Trilinear_interpolation#Method
  float a = ComputeBilinearInterpolationInternal(xx, yy, f000, f001, f010, f011);
  float b = ComputeBilinearInterpolationInternal(xx, yy, f100, f101, f110, f111);

  return (1 - zz) * a + zz * b;
}
