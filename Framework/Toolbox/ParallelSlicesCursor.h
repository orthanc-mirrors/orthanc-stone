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

#include "ParallelSlices.h"
#include "../Enumerations.h"

namespace OrthancStone
{
  class ParallelSlicesCursor : public boost::noncopyable
  {
  private:
    std::auto_ptr<ParallelSlices>  slices_;
    size_t                         currentSlice_;

    size_t GetDefaultSlice();

  public:
    ParallelSlicesCursor() :
      currentSlice_(0)
    {
    }

    void SetGeometry(const ParallelSlices& slices);

    size_t GetSliceCount();

    CoordinateSystem3D GetSlice(size_t slice);

    CoordinateSystem3D GetCurrentSlice();

    // Returns "true" iff. the slice has actually changed
    bool SetDefaultSlice();

    // Returns "true" iff. the slice has actually changed
    bool ApplyOffset(SliceOffsetMode mode,
                     int offset);

    // Returns "true" iff. the slice has actually changed
    bool ApplyWheelEvent(MouseWheelDirection direction,
                         KeyboardModifiers modifiers);

    // Returns "true" iff. the slice has actually changed
    bool LookupSliceContainingPoint(const Vector& p);
  };
}
