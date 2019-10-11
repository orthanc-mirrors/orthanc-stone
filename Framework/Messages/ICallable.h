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
#include "IObserver.h"

#include <Core/Logging.h>

#include <boost/noncopyable.hpp>
#include <boost/weak_ptr.hpp>

#include <string>

namespace OrthancStone 
{
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

    virtual const MessageIdentifier& GetMessageIdentifier() = 0;

    // TODO - Is this needed?
    virtual boost::weak_ptr<IObserver> GetObserver() const = 0;
  };


  // TODO - Remove this class
  template <typename TMessage>
  class MessageHandler : public ICallable
  {
  };


  template <typename TObserver,
            typename TMessage>
  class Callable : public MessageHandler<TMessage>
  {
  private:
    typedef void (TObserver::* MemberFunction) (const TMessage&);

    boost::weak_ptr<IObserver>  observer_;
    MemberFunction              function_;

  public:
    Callable(boost::shared_ptr<TObserver> observer,
             MemberFunction function) :
      observer_(observer),
      function_(function)
    {
    }

    virtual void Apply(const IMessage& message)
    {
      boost::shared_ptr<IObserver> lock(observer_);
      if (lock)
      {
        TObserver& observer = dynamic_cast<TObserver&>(*lock);
        const TMessage& typedMessage = dynamic_cast<const TMessage&>(message);
        (observer.*function_) (typedMessage);
      }
    }

    virtual const MessageIdentifier& GetMessageIdentifier()
    {
      return TMessage::GetStaticIdentifier();
    }

    virtual boost::weak_ptr<IObserver> GetObserver() const
    {
      return observer_;
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

    virtual IObserver* GetObserver() const
    {
      return &observer_;
    }
  };
#endif //__cplusplus >= 201103L
}
