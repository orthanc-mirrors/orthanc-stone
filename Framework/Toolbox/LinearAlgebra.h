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

namespace OrthancStone
{
  typedef boost::numeric::ublas::matrix<double>   Matrix;
  typedef boost::numeric::ublas::vector<double>   Vector;

  namespace LinearAlgebra
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
    
    void FillMatrix(Matrix& target,
                    size_t rows,
                    size_t columns,
                    const double values[]);

    void FillVector(Vector& target,
                    size_t size,
                    const double values[]);

    void Convert(Matrix& target,
                 const Vector& source);
  };
}
