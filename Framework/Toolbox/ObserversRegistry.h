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

#include "../../Resources/Orthanc/Core/OrthancException.h"

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <set>

namespace OrthancStone
{
  template <
    typename Source,
    typename Observer = typename Source::IChangeObserver
    >
  class ObserversRegistry : public boost::noncopyable
  {
  private:
    struct ChangeFunctor : public boost::noncopyable
    {
      void operator() (Observer& observer,
                       const Source& source)
      {
        observer.NotifyChange(source);
      }
    };

    typedef std::set<Observer*>  Observers;

    boost::mutex  mutex_;
    Observers     observers_;
    bool          empty_;

  public:
    ObserversRegistry() : empty_(true)
    {
    }

    template <typename Functor>
    void Notify(const Source* source,
                Functor& functor)
    {
      if (empty_)
      {
        // Optimization to avoid the unnecessary locking of the mutex
        return;
      }

      if (source == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      boost::mutex::scoped_lock lock(mutex_);

      for (typename Observers::const_iterator observer = observers_.begin();
           observer != observers_.end(); ++observer)
      {
        functor(**observer, *source);
      }
    }

    template <typename Functor>
    void Notify(const Source* source)
    {
      // Use the default functor
      Functor functor;
      Notify(source, functor);
    }

    void NotifyChange(const Source* source)
    {
      Notify<ChangeFunctor>(source);
    }

    void Register(Observer& observer)
    {
      empty_ = false;

      boost::mutex::scoped_lock lock(mutex_);
      observers_.insert(&observer);
    }

    void Unregister(Observer& observer)
    {
      boost::mutex::scoped_lock lock(mutex_);
      observers_.erase(&observer);

      if (observers_.empty())
      {
        empty_ = true;
      }
    }

    bool IsEmpty()
    {
#if 0
      boost::mutex::scoped_lock lock(mutex_);
      return observers_.empty();
#else
      return empty_;
#endif
    }
  };
}
