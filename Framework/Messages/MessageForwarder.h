/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "ICallable.h"
#include "IObserver.h"

#include <boost/noncopyable.hpp>

namespace OrthancStone
{

  class IObservable;

  class IMessageForwarder : public IObserver
  {
    IObservable& emitter_;
  public:
    IMessageForwarder(MessageBroker& broker, IObservable& emitter)
      : IObserver(broker),
        emitter_(emitter)
    {}
    virtual ~IMessageForwarder() {}

  protected:
    void ForwardMessageInternal(const IMessage& message);
    void RegisterForwarderInEmitter();

  };

  /* When an Observer (B) simply needs to re-emit a message it has received, instead of implementing
   * a specific member function to forward the message, it can create a MessageForwarder.
   * The MessageForwarder will re-emit the message "in the name of (B)"
   *
   * Consider the chain where
   * A is an observable
   * |
   * B is an observer of A and observable
   * |
   * C is an observer of B and knows that B is re-emitting many messages from A
   *
   * instead of implementing a callback, B will create a MessageForwarder that will emit the messages in his name:
   * A.RegisterObserverCallback(new MessageForwarder<A::MessageType>(broker, *this)  // where this is B
   *
   * in C:
   * B.RegisterObserverCallback(new Callable<C, A:MessageTyper>(*this, &B::MyCallback))   // where this is C
   */
  template<typename TMessage>
  class MessageForwarder : public IMessageForwarder, public Callable<MessageForwarder<TMessage>, TMessage>
  {
  public:
    MessageForwarder(MessageBroker& broker,
                     IObservable& emitter // the object that will emit the messages to forward
                     )
      : IMessageForwarder(broker, emitter),
        Callable<MessageForwarder<TMessage>, TMessage>(*this, &MessageForwarder::ForwardMessage)
    {
      RegisterForwarderInEmitter();
    }

protected:
    void ForwardMessage(const TMessage& message)
    {
      ForwardMessageInternal(message);
    }

  };
}
