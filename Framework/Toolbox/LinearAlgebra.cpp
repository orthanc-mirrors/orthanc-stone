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


#include "LinearAlgebra.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

#include <stdio.h>
#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  namespace LinearAlgebra
  {
    void Print(const Vector& v)
    {
      for (size_t i = 0; i < v.size(); i++)
      {
        printf("%g\n", v[i]);
        //printf("%8.2f\n", v[i]);
      }
      printf("\n");
    }


    void Print(const Matrix& m)
    {
      for (size_t i = 0; i < m.size1(); i++)
      {
        for (size_t j = 0; j < m.size2(); j++)
        {
          printf("%g  ", m(i,j));
          //printf("%8.2f  ", m(i,j));
        }
        printf("\n");        
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
                     const Orthanc::DicomMap& dataset,
                     const Orthanc::DicomTag& tag)
    {
      std::string value;
      return (dataset.CopyToString(value, tag, false) &&
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


    void AssignVector(Vector& v,
                      double v1,
                      double v2,
                      double v3,
                      double v4)
    {
      v.resize(4);
      v[0] = v1;
      v[1] = v2;
      v[2] = v3;
      v[3] = v4;
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


    void FillMatrix(Matrix& target,
                    size_t rows,
                    size_t columns,
                    const double values[])
    {
      target.resize(rows, columns);

      size_t index = 0;

      for (size_t y = 0; y < rows; y++)
      {
        for (size_t x = 0; x < columns; x++, index++)
        {
          target(y, x) = values[index];
        }
      }
    }


    void FillVector(Vector& target,
                    size_t size,
                    const double values[])
    {
      target.resize(size);

      for (size_t i = 0; i < size; i++)
      {
        target[i] = values[i];
      }
    }


    void Convert(Matrix& target,
                 const Vector& source)
    {
      const size_t n = source.size();

      target.resize(n, 1);

      for (size_t i = 0; i < n; i++)
      {
        target(i, 0) = source[i];
      }      
    }

    
    double ComputeDeterminant(const Matrix& a)
    {
      if (a.size1() != a.size2())
      {
        LOG(ERROR) << "Determinant only exists for square matrices";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    
      // https://en.wikipedia.org/wiki/Rule_of_Sarrus
      if (a.size1() == 1)
      {
        return a(0,0);
      }
      else if (a.size1() == 2)
      {
        return a(0,0) * a(1,1) - a(0,1) * a(1,0);
      }
      else if (a.size1() == 3)
      {
        return (a(0,0) * a(1,1) * a(2,2) + 
                a(0,1) * a(1,2) * a(2,0) +
                a(0,2) * a(1,0) * a(2,1) -
                a(2,0) * a(1,1) * a(0,2) -
                a(2,1) * a(1,2) * a(0,0) -
                a(2,2) * a(1,0) * a(0,1));
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }
    

    bool IsOrthogonalMatrix(const Matrix& q,
                            double threshold)
    {
      // https://en.wikipedia.org/wiki/Orthogonal_matrix

      using namespace boost::numeric::ublas;

      const Matrix check = prod(trans(q), q) - identity_matrix<double>(3);

      type_traits<double>::real_type norm = norm_inf(check);

      return (norm <= threshold);
    }


    bool IsOrthogonalMatrix(const Matrix& q)
    {
      return IsOrthogonalMatrix(q, 10.0 * std::numeric_limits<float>::epsilon());
    }


    bool IsRotationMatrix(const Matrix& r,
                          double threshold)
    {
      return (IsOrthogonalMatrix(r, threshold) &&
              IsNear(ComputeDeterminant(r), 1.0, threshold));
    }


    bool IsRotationMatrix(const Matrix& r)
    {
      return IsRotationMatrix(r, 10.0 * std::numeric_limits<float>::epsilon());
    }


    void InvertUpperTriangularMatrix(Matrix& output,
                                     const Matrix& k)
    {
      if (k.size1() != k.size2())
      {
        LOG(ERROR) << "Determinant only exists for square matrices";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      output.resize(k.size1(), k.size2());

      for (size_t i = 1; i < k.size1(); i++)
      {
        for (size_t j = 0; j < i; j++)
        {
          if (!IsCloseToZero(k(i, j)))
          {
            LOG(ERROR) << "Not an upper triangular matrix";
            throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
          }

          output(i, j) = 0;  // The output is also upper triangular
        }
      }

      if (k.size1() == 3)
      {
        // https://math.stackexchange.com/a/1004181
        double a = k(0, 0);
        double b = k(0, 1);
        double c = k(0, 2);
        double d = k(1, 1);
        double e = k(1, 2);
        double f = k(2, 2);

        if (IsCloseToZero(a) ||
            IsCloseToZero(d) ||
            IsCloseToZero(f))
        {
          LOG(ERROR) << "Singular upper triangular matrix";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }
        else
        {
          output(0, 0) = 1.0 / a;
          output(0, 1) = -b / (a * d);
          output(0, 2) = (b * e - c * d) / (a * f * d);
          output(1, 1) = 1.0 / d;
          output(1, 2) = -e / (f * d);
          output(2, 2) = 1.0 / f;
        }
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }
    }


    static void GetGivensComponent(double& c,
                                   double& s,
                                   const Matrix& a,
                                   size_t i,
                                   size_t j)
    {
      assert(i < 3 && j < 3);

      double x = a(i, i);
      double y = a(i, j);
      double n = sqrt(x * x + y * y);

      if (IsCloseToZero(n))
      {
        c = 1;
        s = 0;
      }
      else
      {
        c = x / n;
        s = -y / n;
      }
    }


    /**
     * This function computes the RQ decomposition of a 3x3 matrix,
     * using Givens rotations. Reference: Algorithm A4.1 (page 579) of
     * "Multiple View Geometry in Computer Vision" (2nd edition).  The
     * output matrix "Q" is a rotation matrix, and "R" is upper
     * triangular.
     **/
    void RQDecomposition3x3(Matrix& r,
                            Matrix& q,
                            const Matrix& a)
    {
      using namespace boost::numeric::ublas;
      
      if (a.size1() != 3 ||
          a.size2() != 3)
      {
        LOG(ERROR) << "Only applicable to a 3x3 matrix";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      r.resize(3, 3);
      q.resize(3, 3);

      r = a;
      q = identity_matrix<double>(3);

      {
        // Set A(2,1) to zero
        double c, s;
        GetGivensComponent(c, s, r, 2, 1);

        double v[9] = { 1, 0, 0, 
                        0, c, -s,
                        0, s, c };

        Matrix g;
        FillMatrix(g, 3, 3, v);

        r = prod(r, g);
        q = prod(trans(g), q);
      }


      {
        // Set A(2,0) to zero
        double c, s;
        GetGivensComponent(c, s, r, 2, 0);

        double v[9] = { c, 0, -s, 
                        0, 1, 0,
                        s, 0, c };

        Matrix g;
        FillMatrix(g, 3, 3, v);

        r = prod(r, g);
        q = prod(trans(g), q);
      }


      {
        // Set A(1,0) to zero
        double c, s;
        GetGivensComponent(c, s, r, 1, 0);

        double v[9] = { c, -s, 0, 
                        s, c, 0,
                        0, 0, 1 };

        Matrix g;
        FillMatrix(g, 3, 3, v);

        r = prod(r, g);
        q = prod(trans(g), q);
      }

      if (!IsCloseToZero(norm_inf(prod(r, q) - a)) ||
          !IsRotationMatrix(q) ||
          !IsCloseToZero(r(1, 0)) ||
          !IsCloseToZero(r(2, 0)) ||
          !IsCloseToZero(r(2, 1)))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }
}
