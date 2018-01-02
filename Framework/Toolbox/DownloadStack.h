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

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

namespace OrthancStone
{
  class DownloadStack : public boost::noncopyable
  {
  private:
    static const int NIL = -1;

    // This is a doubly-linked list
    struct Node
    {
      int   next_;
      int   prev_;
      bool  dequeued_;
    };

    boost::mutex        mutex_;
    std::vector<Node>   nodes_;
    int                 firstNode_;

    bool CheckInvariants() const;

    void SetTopNodeInternal(unsigned int value);  

  public:
    DownloadStack(unsigned int size);

    ~DownloadStack();

    bool Pop(unsigned int& value);

    class Writer : public boost::noncopyable
    {
    private:
      DownloadStack&              that_;
      boost::mutex::scoped_lock   lock_;
      
    public:
      Writer(DownloadStack& that) :
        that_(that),
        lock_(that.mutex_)
      {
      }
      
      void SetTopNode(unsigned int value);  

      void SetTopNodePermissive(int value);
    };
  };
}
