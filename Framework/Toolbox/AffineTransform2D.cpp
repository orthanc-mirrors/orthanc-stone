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


#include "AffineTransform2D.h"

#include "ImageGeometry.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  AffineTransform2D::AffineTransform2D() :
    matrix_(LinearAlgebra::IdentityMatrix(3))
  {
  }

  
  AffineTransform2D::AffineTransform2D(const Matrix& m)
  {
    if (m.size1() != 3 ||
        m.size2() != 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
    }

    if (!LinearAlgebra::IsCloseToZero(m(2, 0)) ||
        !LinearAlgebra::IsCloseToZero(m(2, 1)) ||
        LinearAlgebra::IsCloseToZero(m(2, 2)))
    {
      LOG(ERROR) << "Cannot setup an AffineTransform2D with perspective effects";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    matrix_ = m / m(2, 2);
  }
    

  void AffineTransform2D::Apply(double& x /* inout */,
                                double& y /* inout */) const
  {
    Vector p;
    LinearAlgebra::AssignVector(p, x, y, 1);

    Vector q = LinearAlgebra::Product(matrix_, p);

    if (!LinearAlgebra::IsNear(q[2], 1.0))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      x = q[0];
      y = q[1];
    }
  }


  void AffineTransform2D::Apply(Orthanc::ImageAccessor& target,
                                const Orthanc::ImageAccessor& source,
                                ImageInterpolation interpolation,
                                bool clear) const
  {
    assert(LinearAlgebra::IsNear(matrix_(2, 0), 0) &&
           LinearAlgebra::IsNear(matrix_(2, 1), 0) &&
           LinearAlgebra::IsNear(matrix_(2, 2), 1));

    ApplyAffineTransform(target, source,
                         matrix_(0, 0), matrix_(0, 1), matrix_(0, 2),
                         matrix_(1, 0), matrix_(1, 1), matrix_(1, 2),
                         interpolation, clear);
  }


  AffineTransform2D AffineTransform2D::Invert(const AffineTransform2D& a)
  {
    AffineTransform2D t;
    LinearAlgebra::InvertMatrix(t.matrix_, a.matrix_);
    return t;
  }


  AffineTransform2D AffineTransform2D::Combine(const AffineTransform2D& a,
                                               const AffineTransform2D& b)
  {
    return AffineTransform2D(LinearAlgebra::Product(a.GetHomogeneousMatrix(),
                                                    b.GetHomogeneousMatrix()));
  }
  
      
  AffineTransform2D AffineTransform2D::Combine(const AffineTransform2D& a,
                                               const AffineTransform2D& b,
                                               const AffineTransform2D& c)
  {
    return AffineTransform2D(LinearAlgebra::Product(a.GetHomogeneousMatrix(),
                                                    b.GetHomogeneousMatrix(),
                                                    c.GetHomogeneousMatrix()));
  }
  
      
  AffineTransform2D AffineTransform2D::Combine(const AffineTransform2D& a,
                                               const AffineTransform2D& b,
                                               const AffineTransform2D& c,
                                               const AffineTransform2D& d)
  {
    return AffineTransform2D(LinearAlgebra::Product(a.GetHomogeneousMatrix(),
                                                    b.GetHomogeneousMatrix(),
                                                    c.GetHomogeneousMatrix(),
                                                    d.GetHomogeneousMatrix()));
  }
  

  AffineTransform2D AffineTransform2D::CreateOffset(double dx,
                                                    double dy)
  {
    AffineTransform2D t;
    t.matrix_(0, 2) = dx;
    t.matrix_(1, 2) = dy;
      
    return t;
  }
  

  AffineTransform2D AffineTransform2D::CreateScaling(double sx,
                                                     double sy)
  {
    AffineTransform2D t;
    t.matrix_(0, 0) = sx;
    t.matrix_(1, 1) = sy;
      
    return t;
  }
  

  AffineTransform2D AffineTransform2D::CreateRotation(double angle)
  {
    double cosine = cos(angle);
    double sine = sin(angle);
      
    AffineTransform2D t;
    t.matrix_(0, 0) = cosine;
    t.matrix_(0, 1) = -sine;
    t.matrix_(1, 0) = sine;
    t.matrix_(1, 1) = cosine;

    return t;
  }
}
