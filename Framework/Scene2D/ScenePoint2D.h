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


#pragma once

#include "../Toolbox/AffineTransform2D.h"


namespace OrthancStone
{
  class ScenePoint2D
  {
  private:
    double  x_;
    double  y_;

  public:
    ScenePoint2D(double x,
                 double y) :
      x_(x),
      y_(y)
    {
    }

    double GetX() const
    {
      return x_;
    }

    double GetY() const
    {
      return y_;
    }

    ScenePoint2D Apply(const AffineTransform2D& t) const
    {
      double x = x_;
      double y = y_;
      t.Apply(x, y);
      return ScenePoint2D(x, y);
    }
  };
}
