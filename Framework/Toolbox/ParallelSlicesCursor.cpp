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


#include "ParallelSlicesCursor.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  size_t ParallelSlicesCursor::GetDefaultSlice()
  {
    if (slices_.get() == NULL)
    {
      return 0;
    }
    else
    {
      return slices_->GetSliceCount() / 2;
    }
  }


  size_t ParallelSlicesCursor::GetSliceCount()
  {
    if (slices_.get() == NULL)
    {
      return 0;
    }
    else
    {
      return slices_->GetSliceCount();
    }
  }


  CoordinateSystem3D ParallelSlicesCursor::GetSlice(size_t slice)
  {
    if (slices_.get() == NULL)
    {
      return CoordinateSystem3D();
    }
    else
    {
      return slices_->GetSlice(slice);
    }
  }


  void ParallelSlicesCursor::SetGeometry(const ParallelSlices& slices)
  {
    slices_.reset(new ParallelSlices(slices));

    currentSlice_ = GetDefaultSlice();
  }


  CoordinateSystem3D ParallelSlicesCursor::GetCurrentSlice()
  {
    if (slices_.get() != NULL &&
        currentSlice_ < slices_->GetSliceCount())
    {
      return slices_->GetSlice(currentSlice_);
    }
    else
    {
      return CoordinateSystem3D();  // No slice is available, return the canonical geometry
    }
  }


  bool ParallelSlicesCursor::SetDefaultSlice()
  {
    size_t slice = GetDefaultSlice();

    if (currentSlice_ != slice)
    {
      currentSlice_ = slice;
      return true;
    }
    else
    {
      return false;
    }
  }


  bool ParallelSlicesCursor::ApplyOffset(SliceOffsetMode mode,
                                         int offset)
  {
    if (slices_.get() == NULL)
    {
      return false;
    }

    int count = slices_->GetSliceCount();
    if (count == 0)
    {
      return false;
    }

    int slice;
    if (static_cast<int>(currentSlice_) >= count)
    {
      slice = count - 1;
    }
    else
    {
      slice = currentSlice_;
    }

    switch (mode)
    {
      case SliceOffsetMode_Absolute:
      {
        slice = offset;
        break;
      }

      case SliceOffsetMode_Relative:
      {
        slice += offset;
        break;
      }

      case SliceOffsetMode_Loop:
      {
        slice += offset;
        while (slice < 0)
        {
          slice += count;
        }

        while (slice >= count)
        {
          slice -= count;
        }

        break;
      }

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (slice < 0)
    {
      slice = 0;
    }

    if (slice >= count)
    {
      slice = count - 1;
    }

    if (slice != static_cast<int>(currentSlice_))
    {
      currentSlice_ = static_cast<int>(slice);
      return true;
    }
    else
    {
      return false;
    }
  }


  bool ParallelSlicesCursor::ApplyWheelEvent(MouseWheelDirection direction,
                                             KeyboardModifiers modifiers)
  {
    int offset = (modifiers & KeyboardModifiers_Control ? 10 : 1);

    switch (direction)
    {
      case MouseWheelDirection_Down:
        return ApplyOffset(SliceOffsetMode_Relative, -offset);

      case MouseWheelDirection_Up:
        return ApplyOffset(SliceOffsetMode_Relative, offset);

      default:
        return false;
    }
  }


  bool ParallelSlicesCursor::LookupSliceContainingPoint(const Vector& p)
  {
    size_t slice;
    double distance;

    if (slices_.get() != NULL &&
        slices_->ComputeClosestSlice(slice, distance, p))
    {
      if (currentSlice_ != slice)
      {
        currentSlice_ = slice;
        return true;
      }
    }

    return false;
  }
}
