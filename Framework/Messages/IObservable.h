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

#include "MessageBroker.h"
#include <set>

namespace OrthancStone {

  class IObservable : public boost::noncopyable
  {
  protected:
    MessageBroker&                     broker_;

    std::set<IObserver*>              observers_;

  public:

    IObservable(MessageBroker& broker)
      : broker_(broker)
    {
    }
    virtual ~IObservable()
    {
    }

    void EmitMessage(const IMessage& message)
    {
      broker_.EmitMessage(*this, observers_, message);
    }

    void RegisterObserver(IObserver& observer)
    {
      observers_.insert(&observer);
    }

    void UnregisterObserver(IObserver& observer)
    {
      observers_.erase(&observer);
    }
  };

}
