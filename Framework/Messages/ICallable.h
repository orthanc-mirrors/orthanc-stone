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

#include <Core/Logging.h>

#include <boost/noncopyable.hpp>

#include <string>

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

    virtual const MessageIdentifier& GetMessageIdentifier() = 0;

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

    TObserver&         observer_;
    MemberFunction     function_;
    std::string        observerFingerprint_;

  public:
    Callable(TObserver& observer,
             MemberFunction function) :
      observer_(observer),
      observerFingerprint_(observer.GetFingerprint()),
      function_(function)
    {
    }

    void ApplyInternal(const TMessage& message)
    {
      std::string currentFingerprint(observer_.GetFingerprint());
      if (observerFingerprint_ != currentFingerprint)
      {
        LOG(TRACE) << "The observer at address " << 
          std::hex << &observer_ << std::dec << 
          ") has a different fingerprint than the one recorded at callback " <<
          "registration time. This means that it is not the same object as " <<
          "the one recorded, even though their addresses are the same. " <<
          "Callback will NOT be sent!";
        LOG(TRACE) << " recorded fingerprint = " << observerFingerprint_ << 
          " current fingerprint = " << currentFingerprint;
      }
      else
      {
        LOG(TRACE) << "The recorded fingerprint is " << observerFingerprint_ << " and the current fingerprint is " << currentFingerprint << " -- callable will be called.";
        (observer_.*function_) (message);
      }
    }

    virtual void Apply(const IMessage& message)
    {
      ApplyInternal(dynamic_cast<const TMessage&>(message));
    }

    virtual const MessageIdentifier& GetMessageIdentifier()
    {
      return TMessage::GetStaticIdentifier();
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

    virtual IObserver* GetObserver() const
    {
      return &observer_;
    }
  };
#endif //__cplusplus >= 201103L
}
