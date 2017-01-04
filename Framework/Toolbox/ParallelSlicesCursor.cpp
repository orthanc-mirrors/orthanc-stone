/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "ParallelSlicesCursor.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"

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
    boost::mutex::scoped_lock lock(mutex_);

    if (slices_.get() == NULL)
    {
      return 0;
    }
    else
    {
      return slices_->GetSliceCount();
    }
  }


  SliceGeometry ParallelSlicesCursor::GetSlice(size_t slice)
  {
    boost::mutex::scoped_lock lock(mutex_);

    if (slices_.get() == NULL)
    {
      return SliceGeometry();
    }
    else
    {
      return slices_->GetSlice(slice);
    }
  }


  void ParallelSlicesCursor::SetGeometry(const ParallelSlices& slices)
  {
    boost::mutex::scoped_lock lock(mutex_);

    slices_.reset(new ParallelSlices(slices));

    currentSlice_ = GetDefaultSlice();
  }


  SliceGeometry ParallelSlicesCursor::GetCurrentSlice()
  {
    boost::mutex::scoped_lock lock(mutex_);

    if (slices_.get() != NULL &&
        currentSlice_ < slices_->GetSliceCount())
    {
      return slices_->GetSlice(currentSlice_);
    }
    else
    {
      return SliceGeometry();  // No slice is available, return the canonical geometry
    }
  }


  bool ParallelSlicesCursor::SetDefaultSlice()
  {
    boost::mutex::scoped_lock lock(mutex_);

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
    boost::mutex::scoped_lock lock(mutex_);

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
    boost::mutex::scoped_lock lock(mutex_);

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
