/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "SegmentTree.h"

#include <OrthancException.h>

#include <cassert>


namespace OrthancStone
{
  SegmentTree::SegmentTree(size_t lowBound,
                           size_t highBound,
                           IPayloadFactory& factory) :
    lowBound_(lowBound),
    highBound_(highBound),
    payload_(factory.Create())
  {
    if (payload_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    
    if (lowBound >= highBound)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    if (highBound - lowBound > 1)
    {
      size_t middle = (lowBound + highBound) / 2;
      left_.reset(new SegmentTree(lowBound, middle, factory));
      right_.reset(new SegmentTree(middle, highBound, factory));
    }
  }


  Orthanc::IDynamicObject& SegmentTree::GetPayload() const
  {
    assert(payload_.get() != NULL);
    return *payload_;
  }


  SegmentTree& SegmentTree::GetLeftChild() const
  {
    if (IsLeaf())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      assert(left_.get() != NULL);
      return *left_;
    }
  }


  SegmentTree& SegmentTree::GetRightChild() const
  {
    if (IsLeaf())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      assert(right_.get() != NULL);
      return *right_;
    }
  }


  size_t SegmentTree::CountNodes() const
  {
    if (IsLeaf())
    {
      return 1;
    }
    else
    {
      return 1 + GetLeftChild().CountNodes() + GetRightChild().CountNodes();
    }
  }


  void SegmentTree::Visit(size_t low,
                          size_t high,
                          IVisitor& visitor)
  {
    if (low >= high)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    
    assert(payload_.get() != NULL);
    
    // Aliases to use the same variable names as in the textbook
    const size_t& b = low;
    const size_t& e = high;
    const size_t& bv = GetLowBound();
    const size_t& ev = GetHighBound();
    
    if (b <= bv &&
        ev <= e)
    {
      // The interval of this node is fully inside the user-provided interval
      visitor.Visit(*this, true);
    }
    else if (!IsLeaf())
    {
      // The child nodes are first updated
      size_t middle = (bv + ev) / 2;

      if (b < middle)
      {
        GetLeftChild().Visit(b, e, visitor);
      }

      if (middle < e)
      {
        GetRightChild().Visit(b, e, visitor);
      }
      
      // The interval of this node only partially intersects the user-provided interval
      visitor.Visit(*this, false);
    }
  }
}
