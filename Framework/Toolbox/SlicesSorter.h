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


#pragma once

#include "Slice.h"

namespace OrthancStone
{
  class SlicesSorter : public boost::noncopyable
  {
  private:
    class SliceWithDepth;
    struct Comparator;

    typedef std::vector<SliceWithDepth*>  Slices;

    Slices  slices_;
    bool    hasNormal_;
    
  public:
    SlicesSorter() : hasNormal_(false)
    {
    }

    ~SlicesSorter();
    
    void Reserve(size_t count)
    {
      slices_.reserve(count);
    }

    void AddSlice(const Slice& slice);

    size_t GetSliceCount() const
    {
      return slices_.size();
    }

    const Slice& GetSlice(size_t i) const;

    void SetNormal(const Vector& normal);
    
    void Sort();

    void FilterNormal(const Vector& normal);
    
    bool SelectNormal(Vector& normal) const;
  };
}
