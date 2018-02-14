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
  }
}
