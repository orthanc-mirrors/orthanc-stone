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


#include "gtest/gtest.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <string>
#include <map>
#include <set>

int testCounter = 0;
namespace {

  enum MessageType
  {
    MessageType_Test1,
    MessageType_Test2,

    MessageType_CustomMessage,
    MessageType_LastGenericStoneMessage
  };

  struct IMessage  : public boost::noncopyable
  {
    MessageType messageType_;
  public:
    IMessage(const MessageType& messageType)
      : messageType_(messageType)
    {}
    virtual ~IMessage() {}

    virtual int GetType() const {return messageType_;}
  };


  struct ICustomMessage  : public IMessage
  {
    int customMessageType_;
  public:
    ICustomMessage(int customMessageType)
      : IMessage(MessageType_CustomMessage),
        customMessageType_(customMessageType)
    {}
    virtual ~ICustomMessage() {}

    virtual int GetType() const {return customMessageType_;}
  };


  class IObserver;
  class IObservable;
  class IPromiseTarget;
  class IPromiseSource;
  class Promise;

  /*
   * This is a central message broker.  It keeps track of all observers and knows
   * when an observer is deleted.
   * This way, it can prevent an observable to send a message to a delete observer.
   * It does the same book-keeping for the IPromiseTarget and IPromiseSource
   */
  class MessageBroker : public boost::noncopyable
  {

    std::set<IObserver*> activeObservers_;  // the list of observers that are currently alive (that have not been deleted)
    std::set<IPromiseTarget*> activePromiseTargets_;
    std::set<IPromiseSource*> activePromiseSources_;

  public:

    void Register(IObserver& observer)
    {
      activeObservers_.insert(&observer);
    }

    void Unregister(IObserver& observer)
    {
      activeObservers_.erase(&observer);
    }

    void Register(IPromiseTarget& target)
    {
      activePromiseTargets_.insert(&target);
    }

    void Unregister(IPromiseTarget& target)
    {
      activePromiseTargets_.erase(&target);
    }

    void Register(IPromiseSource& source)
    {
      activePromiseSources_.insert(&source);
    }

    void Unregister(IPromiseSource& source)
    {
      activePromiseSources_.erase(&source);
    }

    void EmitMessage(IObservable& from, std::set<IObserver*> observers, const IMessage& message);

    bool IsActive(IPromiseTarget* target)
    {
      return activePromiseTargets_.find(target) != activePromiseTargets_.end();
    }

    bool IsActive(IPromiseSource* source)
    {
      return activePromiseSources_.find(source) != activePromiseSources_.end();
    }

    bool IsActive(IObserver* observer)
    {
      return activeObservers_.find(observer) != activeObservers_.end();
    }
  };

  struct IPromiseArgs
  {
public:
    virtual ~IPromiseArgs() {}
  };

  class EmptyPromiseArguments : public IPromiseArgs
  {

  };

  class Promise : public boost::noncopyable
  {
  protected:
    MessageBroker&                    broker_;

    IPromiseTarget*                                           successTarget_;
    boost::function<void (const IPromiseArgs& message)>       successCallable_;

    IPromiseTarget*                                           failureTarget_;
    boost::function<void (const IPromiseArgs& message)>       failureCallable_;

  public:
    Promise(MessageBroker& broker)
      : broker_(broker),
        successTarget_(NULL),
        failureTarget_(NULL)
    {
    }

    void Success(const IPromiseArgs& message)
    {
      // check the target is still alive in the broker
      if (broker_.IsActive(successTarget_))
      {
        successCallable_(message);
      }
    }

    void Failure(const IPromiseArgs& message)
    {
      // check the target is still alive in the broker
      if (broker_.IsActive(failureTarget_))
      {
        failureCallable_(message);
      }
    }

    Promise& Then(IPromiseTarget* target, boost::function<void (const IPromiseArgs& message)> f)
    {
      if (successTarget_ != NULL)
      {
        // TODO: throw throw new "Promise may only have a single success target"
      }
      successTarget_ = target;
      successCallable_ = f;
      return *this;
    }

    Promise& Else(IPromiseTarget* target, boost::function<void (const IPromiseArgs& message)> f)
    {
      if (failureTarget_ != NULL)
      {
        // TODO: throw throw new "Promise may only have a single failure target"
      }
      failureTarget_ = target;
      failureCallable_ = f;
      return *this;
    }

  };

  class IObserver : public boost::noncopyable
  {
  protected:
    MessageBroker&                    broker_;

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

  };

  class IPromiseTarget : public boost::noncopyable
  {
  protected:
    MessageBroker&                    broker_;

  public:
    IPromiseTarget(MessageBroker& broker)
      : broker_(broker)
    {
      broker_.Register(*this);
    }

    virtual ~IPromiseTarget()
    {
      broker_.Unregister(*this);
    }
  };

  class IPromiseSource : public boost::noncopyable
  {
  protected:
    MessageBroker&                    broker_;

  public:
    IPromiseSource(MessageBroker& broker)
      : broker_(broker)
    {
      broker_.Register(*this);
    }

    virtual ~IPromiseSource()
    {
      broker_.Unregister(*this);
    }
  };


  struct CallableObserver
  {
    IObserver* observer;
    boost::function<void (IObservable& from, const IMessage& message)> f;
  };

  class IObservable : public boost::noncopyable
  {
  protected:
    MessageBroker&                     broker_;

    std::set<IObserver*>              observers_;

    std::map<int, std::set<CallableObserver*> > callables_;
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
      //broker_.EmitMessage(*this, observers_, message);
      int messageType = message.GetType();
      if (callables_.find(messageType) != callables_.end())
      {
        for (std::set<CallableObserver*>::iterator observer = callables_[messageType].begin(); observer != callables_[messageType].end(); observer++)
        {
          CallableObserver* callable = *observer;
          if (broker_.IsActive(callable->observer))
          {
            callable->f(*this, message);
          }
        }
      }

    }

    void RegisterObserver(IObserver& observer)
    {
      observers_.insert(&observer);
    }

    void UnregisterObserver(IObserver& observer)
    {
      observers_.erase(&observer);
    }

    //template<typename TObserver> void Connect(MessageType messageType, IObserver& observer, void (TObserver::*ptrToMemberHandler)(IObservable& from, const IMessage& message))
    void Connect(int messageType, IObserver& observer, boost::function<void (IObservable& from, const IMessage& message)> f)
    {
      callables_[messageType] = std::set<CallableObserver*>();
      CallableObserver* callable = new CallableObserver();
      callable->observer = &observer;
      callable->f = f;
      callables_[messageType].insert(callable);
    }
  };


  enum CustomMessageType
  {
    CustomMessageType_First = MessageType_LastGenericStoneMessage + 1,

    CustomMessageType_Completed
  };

  class MyObservable : public IObservable
  {
  public:
    struct MyCustomMessage: public ICustomMessage
    {
      int payload_;
      MyCustomMessage(int payload)
        : ICustomMessage(CustomMessageType_Completed),
          payload_(payload)
      {}
    };

    MyObservable(MessageBroker& broker)
      : IObservable(broker)
    {}

  };

  class MyObserver : public IObserver
  {
  public:
    MyObserver(MessageBroker& broker)
      : IObserver(broker)
    {}
    void HandleCompletedMessage(IObservable& from, const IMessage& message)
    {
      const MyObservable::MyCustomMessage& msg = dynamic_cast<const MyObservable::MyCustomMessage&>(message);
      testCounter += msg.payload_;
    }

  };


  class MyPromiseSource : public IPromiseSource
  {
    Promise* currentPromise_;
  public:
    struct MyPromiseArgs : public IPromiseArgs
    {
      int increment;
    };

    MyPromiseSource(MessageBroker& broker)
      : IPromiseSource(broker),
        currentPromise_(NULL)
    {}

    Promise& StartSomethingAsync()
    {
      currentPromise_ = new Promise(broker_);
      return *currentPromise_;
    }

    void CompleteSomethingAsyncWithSuccess()
    {
      currentPromise_->Success(EmptyPromiseArguments());
      delete currentPromise_;
    }

    void CompleteSomethingAsyncWithFailure()
    {
      currentPromise_->Failure(EmptyPromiseArguments());
      delete currentPromise_;
    }
  };


  class MyPromiseTarget : public IPromiseTarget
  {
  public:
    MyPromiseTarget(MessageBroker& broker)
      : IPromiseTarget(broker)
    {}

    void IncrementCounter(const IPromiseArgs& args)
    {
      testCounter++;
    }

    void DecrementCounter(const IPromiseArgs& args)
    {
      testCounter--;
    }
  };
}

#define CONNECT_MESSAGES(observablePtr, messageType, observerPtr, observerFnPtr) (observablePtr)->Connect(messageType, *(observerPtr), boost::bind(observerFnPtr, observerPtr, _1, _2))
#define PTHEN(targetPtr, targetFnPtr) Then(targetPtr, boost::bind(targetFnPtr, targetPtr, _1))
#define PELSE(targetPtr, targetFnPtr) Else(targetPtr, boost::bind(targetFnPtr, targetPtr, _1))


TEST(MessageBroker2, TestPermanentConnectionSimpleUseCase)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyObserver    observer(broker);

  // create a permanent connection between an observable and an observer
  CONNECT_MESSAGES(&observable, CustomMessageType_Completed, &observer, &MyObserver::HandleCompletedMessage);

  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(12, testCounter);

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(20, testCounter);
}

TEST(MessageBroker2, TestPermanentConnectionDeleteObserver)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyObserver*   observer = new MyObserver(broker);

  // create a permanent connection between an observable and an observer
  CONNECT_MESSAGES(&observable, CustomMessageType_Completed, observer, &MyObserver::HandleCompletedMessage);

  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(12, testCounter);

  // delete the observer and check that the callback is not called anymore
  delete observer;

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(0, testCounter);
}


TEST(MessageBroker2, TestPromiseSuccessFailure)
{
  MessageBroker broker;
  MyPromiseSource  source(broker);
  MyPromiseTarget target(broker);

  // test a successful promise
  source.StartSomethingAsync()
      .PTHEN(&target, &MyPromiseTarget::IncrementCounter)
      .PELSE(&target, &MyPromiseTarget::DecrementCounter);

  testCounter = 0;
  source.CompleteSomethingAsyncWithSuccess();
  ASSERT_EQ(1, testCounter);

  // test a failing promise
  source.StartSomethingAsync()
      .PTHEN(&target, &MyPromiseTarget::IncrementCounter)
      .PELSE(&target, &MyPromiseTarget::DecrementCounter);

  testCounter = 0;
  source.CompleteSomethingAsyncWithFailure();
  ASSERT_EQ(-1, testCounter);
}

//TEST(MessageBroker2, TestPromiseDeleteTarget)
//{
//  MessageBroker broker;
//  MyPromiseSource  source(broker);
//  MyPromiseTarget target(broker);

//  // test a successful promise
//  source.StartSomethingAsync()
//      .PTHEN(&target, &MyPromiseTarget::IncrementCounter)
//      .PELSE(&target, &MyPromiseTarget::DecrementCounter);

//  testCounter = 0;
//  source.CompleteSomethingAsyncWithSuccess();
//  ASSERT_EQ(1, testCounter);

//  // test a failing promise
//  source.StartSomethingAsync()
//      .PTHEN(&target, &MyPromiseTarget::IncrementCounter)
//      .PELSE(&target, &MyPromiseTarget::DecrementCounter);

//  testCounter = 0;
//  source.CompleteSomethingAsyncWithFailure();
//  ASSERT_EQ(-1, testCounter);
//}
