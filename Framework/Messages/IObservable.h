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

#include "../StoneEnumerations.h"
#include "ICallable.h"
#include "IObserver.h"
#include "MessageBroker.h"
#include "MessageForwarder.h"

#include <set>
#include <map>

namespace OrthancStone 
{
  class IObservable : public boost::noncopyable
  {
  private:
    typedef std::map<int, std::set<ICallable*> >  Callables;
    typedef std::set<IMessageForwarder*>          Forwarders;

    MessageBroker&  broker_;
    Callables       callables_;
    Forwarders      forwarders_;

  public:
    IObservable(MessageBroker& broker) :
      broker_(broker)
    {
    }

    virtual ~IObservable()
    {
      // delete all callables (this will also unregister them from the broker)
      for (Callables::const_iterator it = callables_.begin();
           it != callables_.end(); ++it)
      {
        for (std::set<ICallable*>::const_iterator
               it2 = it->second.begin(); it2 != it->second.end(); ++it2)
        {
          delete *it2;
        }
      }

      // unregister the forwarders but don't delete them (they'll be deleted by the observable they are observing as any other callable)
      for (Forwarders::iterator it = forwarders_.begin();
           it != forwarders_.end(); ++it)
      {
        IMessageForwarder* fw = *it;
        broker_.Unregister(dynamic_cast<IObserver&>(*fw));
      }
    }

    void RegisterObserverCallback(ICallable* callable)
    {
      MessageType messageType = callable->GetMessageType();

      callables_[messageType].insert(callable);
    }

    void EmitMessage(const IMessage& message)
    {
      Callables::const_iterator found = callables_.find(message.GetType());

      if (found != callables_.end())
      {
        for (std::set<ICallable*>::const_iterator
               it = found->second.begin(); it != found->second.end(); ++it)
        {
          if (broker_.IsActive((*it)->GetObserver()))
          {
            (*it)->Apply(message);
          }
        }
      }
    }

    void RegisterForwarder(IMessageForwarder* forwarder)
    {
      forwarders_.insert(forwarder);
    }

    MessageBroker& GetBroker() const
    {
      return broker_;
    }
  };
}
