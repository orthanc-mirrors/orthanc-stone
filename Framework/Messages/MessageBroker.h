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

#include "boost/noncopyable.hpp"

#include <set>

namespace OrthancStone
{
  class IObserver;

  /*
   * This is a central message broker.  It keeps track of all observers and knows
   * when an observer is deleted.
   * This way, it can prevent an observable to send a message to a deleted observer.
   */
  class MessageBroker : public boost::noncopyable
  {
  private:
    std::set<const IObserver*> activeObservers_;  // the list of observers that are currently alive (that have not been deleted)

  public:
    void Register(const IObserver& observer)
    {
      activeObservers_.insert(&observer);
    }

    void Unregister(const IObserver& observer)
    {
      activeObservers_.erase(&observer);
    }

    bool IsActive(const IObserver& observer)
    {
      return activeObservers_.find(&observer) != activeObservers_.end();
    }
  };
}
