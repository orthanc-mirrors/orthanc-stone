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


#include "DownloadStack.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"

#include <cassert>

namespace OrthancStone
{
  bool DownloadStack::CheckInvariants() const
  {
    std::vector<bool> dequeued(nodes_.size(), true);

    int i = firstNode_;
    while (i != NIL)
    {
      const Node& node = nodes_[i];

      dequeued[i] = false;

      if (node.next_ != NIL &&
          nodes_[node.next_].prev_ != i)
      {
        return false;
      }

      if (node.prev_ != NIL &&
          nodes_[node.prev_].next_ != i)
      {
        return false;
      }

      i = nodes_[i].next_;
    }

    for (size_t i = 0; i < nodes_.size(); i++)
    {
      if (nodes_[i].dequeued_ != dequeued[i])
      {
        return false;
      }
    }

    return true;
  }


  DownloadStack::DownloadStack(unsigned int size)
  {
    nodes_.resize(size);

    if (size == 0)
    {
      firstNode_ = NIL;
    }
    else
    {
      for (size_t i = 0; i < size; i++)
      {
        nodes_[i].prev_ = i - 1;
        nodes_[i].next_ = i + 1;
        nodes_[i].dequeued_ = false;
      }

      nodes_.front().prev_ = NIL;
      nodes_.back().next_ = NIL;
      firstNode_ = 0;
    }

    assert(CheckInvariants());
  }


  DownloadStack::~DownloadStack()
  {
    assert(CheckInvariants());    
  }


  bool DownloadStack::Pop(unsigned int& value)
  {
    assert(CheckInvariants());

    if (firstNode_ == NIL)
    {
      for (size_t i = 0; i < nodes_.size(); i++)
      {
        assert(nodes_[i].dequeued_);
      }

      return false;
    }
    else
    {
      assert(firstNode_ >= 0 && firstNode_ < static_cast<int>(nodes_.size()));
      value = firstNode_;

      Node& node = nodes_[firstNode_];
      assert(node.prev_ == NIL);
      assert(!node.dequeued_);

      node.dequeued_ = true;
      firstNode_ = node.next_;

      if (firstNode_ != NIL)
      {
        nodes_[firstNode_].prev_ = NIL;
      }

      return true;
    }
  }


  void DownloadStack::SetTopNodeInternal(unsigned int value)
  {
    assert(CheckInvariants());

    Node& node = nodes_[value];

    if (node.dequeued_)
    {
      // This node has already been processed by the download thread, nothing to do
      return;
    }

    // Remove the node from the list
    if (node.prev_ == NIL)
    {
      assert(firstNode_ == static_cast<int>(value));
      
      // This is already the top node in the list, nothing to do
      return;
    }

    nodes_[node.prev_].next_ = node.next_;

    if (node.next_ != NIL)
    {
      nodes_[node.next_].prev_ = node.prev_;
    }

    // Add back the node at the top of the list
    assert(firstNode_ != NIL);

    Node& old = nodes_[firstNode_];
    assert(old.prev_ == NIL);
    assert(!old.dequeued_);
    node.prev_ = NIL;
    node.next_ = firstNode_;
    old.prev_ = value;

    firstNode_ = value;
  }

  
  void DownloadStack::Writer::SetTopNode(unsigned int value)
  {
    if (value >= that_.nodes_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    that_.SetTopNodeInternal(value);
  }


  void DownloadStack::Writer::SetTopNodePermissive(int value)
  {
    if (value >= 0 &&
        value < static_cast<int>(that_.nodes_.size()))
    {
      that_.SetTopNodeInternal(value);
    }
  }
}
