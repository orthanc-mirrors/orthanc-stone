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
#include "IMessage.h"
#include <set>
#include <assert.h>

namespace OrthancStone {

  class IObservable;

  class IObserver : public boost::noncopyable
  {
  protected:
    MessageBroker&                    broker_;
    std::set<MessageType>             handledMessages_;
    std::set<MessageType>             ignoredMessages_;

  public:
    IObserver(MessageBroker& broker)
      : broker_(broker)
    {
      broker_.Register(*this);
    }

    virtual ~IObserver()
    {
      broker_.Unregister(*this);
    }

    void HandleMessage_(const IObservable &from, const IMessage &message)
    {
      assert(handledMessages_.find(message.GetType()) != handledMessages_.end()); // please declare the messages that you're handling

      HandleMessage(from, message);
    }

    virtual void HandleMessage(const IObservable& from, const IMessage& message) = 0;


    const std::set<MessageType>& GetHandledMessages() const
    {
      return handledMessages_;
    }

    const std::set<MessageType>& GetIgnoredMessages() const
    {
      return ignoredMessages_;
    }

  protected:

    // when you connect an IObserver to an IObservable, the observer must handle all observable messages (this is checked during the registration)
    // so, all messages that may be emitted by the observable must be declared "handled" or "ignored" by the observer
    void DeclareHandledMessage(MessageType messageType)
    {
      handledMessages_.insert(messageType);
    }

    void DeclareIgnoredMessage(MessageType messageType)
    {
      ignoredMessages_.insert(messageType);
    }

  };

}
