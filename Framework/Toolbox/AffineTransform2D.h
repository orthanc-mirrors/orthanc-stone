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

#include "../StoneEnumerations.h"
#include "LinearAlgebra.h"

#include <Core/Images/ImageAccessor.h>

namespace OrthancStone
{
  class AffineTransform2D
  {
  private:
    Matrix  matrix_;

  public:
    AffineTransform2D();

    // The matrix must be 3x3, without perspective effects
    AffineTransform2D(const Matrix& m);

    AffineTransform2D(const AffineTransform2D& other) :
      matrix_(other.matrix_)
    {
    }
    
    const Matrix& GetHomogeneousMatrix() const
    {
      return matrix_;
    }

    void Apply(double& x /* inout */,
               double& y /* inout */) const;

    void Apply(Orthanc::ImageAccessor& target,
               const Orthanc::ImageAccessor& source,
               ImageInterpolation interpolation,
               bool clear) const;
    
    static AffineTransform2D Invert(const AffineTransform2D& a);

    static AffineTransform2D Combine(const AffineTransform2D& a,
                                     const AffineTransform2D& b);
      
    static AffineTransform2D Combine(const AffineTransform2D& a,
                                     const AffineTransform2D& b,
                                     const AffineTransform2D& c);
      
    static AffineTransform2D Combine(const AffineTransform2D& a,
                                     const AffineTransform2D& b,
                                     const AffineTransform2D& c,
                                     const AffineTransform2D& d);

    static AffineTransform2D CreateOffset(double dx,
                                          double dy);

    static AffineTransform2D CreateScaling(double sx,
                                           double sy);
    
    static AffineTransform2D CreateRotation(double angle);
  };
}