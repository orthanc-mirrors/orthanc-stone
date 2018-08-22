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

#include "MessageBroker.h"
#include "MessageType.h"
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

    std::set<IObserver*>              observers_;
    std::set<MessageType>             emittableMessages_;

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
      if (emittableMessages_.find(message.GetType()) == emittableMessages_.end())
      {
        throw MessageNotDeclaredException(message.GetType());
      }

      broker_.EmitMessage(*this, observers_, message);
    }

    void RegisterObserver(IObserver& observer)
    {
      CheckObserverDeclaredAllObservableMessages(observer);
      observers_.insert(&observer);
    }

    void UnregisterObserver(IObserver& observer)
    {
      observers_.erase(&observer);
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

    void CheckObserverDeclaredAllObservableMessages(IObserver& observer)
    {
      for (std::set<MessageType>::const_iterator it = emittableMessages_.begin(); it != emittableMessages_.end(); it++)
      {
        // the observer must have "declared" all observable messages
        if (observer.GetHandledMessages().find(*it) == observer.GetHandledMessages().end()
            && observer.GetIgnoredMessages().find(*it) == observer.GetIgnoredMessages().end())
        {
          throw MessageNotDeclaredException(*it);
        }
      }
    }
  };

}
