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

#include <set>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <map>

#include "MessageBroker.h"
#include "MessageType.h"
#include "ICallable.h"
#include "IObserver.h"

namespace OrthancStone {

  class MessageNotDeclaredException : public std::logic_error
  {
    MessageType messageType_;
  public:
    MessageNotDeclaredException(MessageType messageType)
      : std::logic_error("Message not declared by observer."),
        messageType_(messageType)
    {
    }
  };

  class IObservable : public boost::noncopyable
  {
  protected:
    MessageBroker&                     broker_;

    typedef std::map<int, std::set<ICallable*> >   Callables;
    Callables                         callables_;
    std::set<MessageType>             emittableMessages_;

  public:

    IObservable(MessageBroker& broker)
      : broker_(broker)
    {
    }
    virtual ~IObservable()
    {
      for (Callables::const_iterator it = callables_.begin();
           it != callables_.end(); ++it)
      {
        for (std::set<ICallable*>::const_iterator
               it2 = it->second.begin(); it2 != it->second.end(); ++it2)
        {
          delete *it2;
        }
      }
    }

    void RegisterObserverCallback(ICallable* callable)
    {
      MessageType messageType = callable->GetMessageType();

      callables_[messageType].insert(callable);
    }

    void EmitMessage(const IMessage& message)
    {
      if (emittableMessages_.find(message.GetType()) == emittableMessages_.end())
      {
        throw MessageNotDeclaredException(message.GetType());
      }

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
    const std::set<MessageType>& GetEmittableMessages() const
    {
      return emittableMessages_;
    }
  protected:

    void DeclareEmittableMessage(MessageType messageType)
    {
      emittableMessages_.insert(messageType);
    }

  };

}
