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
#include <set>

namespace OrthancStone
{
  template <
    typename Source,
    typename Observer = typename Source::IObserver
    >
  class ObserversRegistry : public boost::noncopyable
  {
  private:
    typedef std::set<Observer*>  Observers;

    Observers  observers_;

  public:
    template <typename Functor>
    void Notify(const Source& source,
                Functor& functor)
    {
      for (typename Observers::const_iterator observer = observers_.begin();
           observer != observers_.end(); ++observer)
      {
        functor(**observer, source);
      }
    }

    void Register(Observer& observer)
    {
      observers_.insert(&observer);
    }

    void Unregister(Observer& observer)
    {
      observers_.erase(&observer);
    }

    bool IsEmpty()
    {
      return observers_.empty();
    }

    void Apply(const Source& source,
               void (Observer::*method) (const Source&))
    {
      for (typename Observers::const_iterator it = observers_.begin();
           it != observers_.end(); ++it)
      {
        ((*it)->*method) (source);
      }
    }

    template <typename Argument0>
    void Apply(const Source& source,
               void (Observer::*method) (const Source&, const Argument0&),
               const Argument0& argument0)
    {
      for (typename Observers::const_iterator it = observers_.begin();
           it != observers_.end(); ++it)
      {
        ((*it)->*method) (source, argument0);
      }
    }

    template <typename Argument0,
              typename Argument1>
    void Apply(const Source& source,
               void (Observer::*method) (const Source&, const Argument0&, const Argument1&),
               const Argument0& argument0,
               const Argument1& argument1)
    {
      for (typename Observers::const_iterator it = observers_.begin();
           it != observers_.end(); ++it)
      {
        ((*it)->*method) (source, argument0, argument1);
      }
    }
  };
}
