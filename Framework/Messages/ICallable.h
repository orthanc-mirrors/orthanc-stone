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

#include "IMessage.h"

#include <boost/noncopyable.hpp>

namespace OrthancStone {

  class IObserver;

  // This is referencing an object and member function that can be notified
  // by an IObservable.  The object must derive from IO
  // The member functions must be of type "void Function(const IMessage& message)" or reference a derived class of IMessage
  class ICallable : public boost::noncopyable
  {
  public:
    virtual ~ICallable()
    {
    }

    virtual void Apply(const IMessage& message) = 0;

    virtual MessageType GetMessageType() const = 0;
    virtual IObserver* GetObserver() const = 0;
  };

  template <typename TMessage>
  class MessageHandler: public ICallable
  {
  };


  template <typename TObserver,
            typename TMessage>
  class Callable : public MessageHandler<TMessage>
  {
  private:
    typedef void (TObserver::* MemberFunction) (const TMessage&);

    TObserver&      observer_;
    MemberFunction  function_;

  public:
    Callable(TObserver& observer,
             MemberFunction function) :
      observer_(observer),
      function_(function)
    {
    }

    void ApplyInternal(const TMessage& message)
    {
      (observer_.*function_) (message);
    }

    virtual void Apply(const IMessage& message)
    {
      ApplyInternal(dynamic_cast<const TMessage&>(message));
    }

    virtual MessageType GetMessageType() const
    {
      return static_cast<MessageType>(TMessage::Type);
    }

    virtual IObserver* GetObserver() const
    {
      return &observer_;
    }
  };

#if 0 /* __cplusplus >= 201103L*/

#include <functional>

  template <typename TMessage>
  class LambdaCallable : public MessageHandler<TMessage>
  {
  private:

    IObserver&      observer_;
    std::function<void (const TMessage&)> lambda_;

  public:
    LambdaCallable(IObserver& observer,
                    std::function<void (const TMessage&)> lambdaFunction) :
             observer_(observer),
             lambda_(lambdaFunction)
    {
    }

    virtual void Apply(const IMessage& message)
    {
      lambda_(dynamic_cast<const TMessage&>(message));
    }

    virtual MessageType GetMessageType() const
    {
      return static_cast<MessageType>(TMessage::Type);
    }

    virtual IObserver* GetObserver() const
    {
      return &observer_;
    }
  };
#endif //__cplusplus >= 201103L
}
