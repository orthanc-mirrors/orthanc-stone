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


#include "Extent.h"

#include <algorithm>
#include <cassert>

namespace OrthancStone
{
  void Extent::Reset()
  {
    empty_ = true;
    x1_ = 0;
    y1_ = 0;
    x2_ = 0;
    y2_ = 0;      
  }

  void Extent::AddPoint(double x,
                        double y)
  {
    if (empty_)
    {
      x1_ = x;
      y1_ = y;
      x2_ = x;
      y2_ = y;
      empty_ = false;
    }
    else
    {
      x1_ = std::min(x1_, x);
      y1_ = std::min(y1_, y);
      x2_ = std::max(x2_, x);
      y2_ = std::max(y2_, y);
    }

    assert(x1_ <= x2_ &&
           y1_ <= y2_);    // This is the invariant of the structure
  }


  void Extent::Union(const Extent& other)
  {
    if (other.IsEmpty())
    {
      return;
    }

    if (IsEmpty())
    {
      *this = other;
      return;
    }

    assert(!IsEmpty());

    x1_ = std::min(x1_, other.x1_);
    y1_ = std::min(y1_, other.y1_);
    x2_ = std::min(x2_, other.x2_);
    y2_ = std::min(y2_, other.y2_);

    assert(x1_ <= x2_ &&
           y1_ <= y2_);    // This is the invariant of the structure
  }

}
