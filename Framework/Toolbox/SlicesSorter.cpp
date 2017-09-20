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


#include "SlicesSorter.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  class SlicesSorter::SliceWithDepth : public boost::noncopyable
  {
  private:
    Slice   slice_;
    double  depth_;

  public:
    SliceWithDepth(const Slice& slice) :
      slice_(slice),
      depth_(0)
    {
    }

    void SetNormal(const Vector& normal)
    {
      depth_ = boost::numeric::ublas::inner_prod
        (slice_.GetGeometry().GetOrigin(), normal);
    }

    double GetDepth() const
    {
      return depth_;
    }

    const Slice& GetSlice() const
    {
      return slice_;
    }
  };


  struct SlicesSorter::Comparator
  {
    bool operator() (const SliceWithDepth* const& a,
                     const SliceWithDepth* const& b) const
    {
      return a->GetDepth() < b->GetDepth();
    }
  };


  SlicesSorter::~SlicesSorter()
  {
    for (size_t i = 0; i < slices_.size(); i++)
    {
      assert(slices_[i] != NULL);
      delete slices_[i];
    }
  }


  void SlicesSorter::AddSlice(const Slice& slice)
  {
    slices_.push_back(new SliceWithDepth(slice));
  }

  
  const Slice& SlicesSorter::GetSlice(size_t i) const
  {
    if (i >= slices_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    assert(slices_[i] != NULL);
    return slices_[i]->GetSlice();
  }

  
  void SlicesSorter::SetNormal(const Vector& normal)
  {
    for (size_t i = 0; i < slices_.size(); i++)
    {
      slices_[i]->SetNormal(normal);
    }

    hasNormal_ = true;
  }
  
    
  void SlicesSorter::Sort()
  {
    if (!hasNormal_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    Comparator comparator;
    std::sort(slices_.begin(), slices_.end(), comparator);
  }
  

  void SlicesSorter::FilterNormal(const Vector& normal)
  {
    size_t pos = 0;

    for (size_t i = 0; i < slices_.size(); i++)
    {
      if (GeometryToolbox::IsParallel(normal, slices_[i]->GetSlice().GetGeometry().GetNormal()))
      {
        // This slice is compatible with the selected normal
        slices_[pos] = slices_[i];
        pos += 1;
      }
      else
      {
        delete slices_[i];
        slices_[i] = NULL;
      }
    }

    slices_.resize(pos);
  }
  
    
  bool SlicesSorter::SelectNormal(Vector& normal) const
  {
    std::vector<Vector>  normalCandidates;
    std::vector<unsigned int>  normalCount;

    bool found = false;

    for (size_t i = 0; !found && i < GetSliceCount(); i++)
    {
      const Vector& normal = GetSlice(i).GetGeometry().GetNormal();

      bool add = true;
      for (size_t j = 0; add && j < normalCandidates.size(); j++)  // (*)
      {
        if (GeometryToolbox::IsParallel(normal, normalCandidates[j]))
        {
          normalCount[j] += 1;
          add = false;
        }
      }

      if (add)
      {
        if (normalCount.size() > 2)
        {
          // To get linear-time complexity in (*). This heuristics
          // allows the series to have one single frame that is
          // not parallel to the others (such a frame could be a
          // generated preview)
          found = false;
        }
        else
        {
          normalCandidates.push_back(normal);
          normalCount.push_back(1);
        }
      }
    }

    for (size_t i = 0; !found && i < normalCandidates.size(); i++)
    {
      unsigned int count = normalCount[i];
      if (count == GetSliceCount() ||
          count + 1 == GetSliceCount())
      {
        normal = normalCandidates[i];
        found = true;
      }
    }

    return found;
  }


  bool SlicesSorter::LookupSlice(size_t& index,
                                 const CoordinateSystem3D& slice) const
  {
    // TODO Turn this linear-time lookup into a log-time lookup,
    // keeping track of whether the slices are sorted along the normal

    for (size_t i = 0; i < slices_.size(); i++)
    {
      if (slices_[i]->GetSlice().ContainsPlane(slice))
      {
        index = i;
        return true;
      }
    }

    return false;
  }
}
