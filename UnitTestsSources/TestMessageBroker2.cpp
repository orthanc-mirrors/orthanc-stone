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


#include "gtest/gtest.h"

#include "Framework/Messages/MessageBroker.h"
#include "Framework/Messages/Promise.h"
#include "Framework/Messages/IObservable.h"
#include "Framework/Messages/IObserver.h"
#include "Framework/Messages/MessageForwarder.h"


int testCounter = 0;
namespace {

//  class IObserver;
//  class IObservable;
//  class Promise;

//  enum MessageType
//  {
//    MessageType_Test1,
//    MessageType_Test2,

//    MessageType_CustomMessage,
//    MessageType_LastGenericStoneMessage
//  };

//  struct IMessage  : public boost::noncopyable
//  {
//    MessageType messageType_;
//  public:
//    IMessage(const MessageType& messageType)
//      : messageType_(messageType)
//    {}
//    virtual ~IMessage() {}

//    virtual int GetType() const {return messageType_;}
//  };


//  struct ICustomMessage  : public IMessage
//  {
//    int customMessageType_;
//  public:
//    ICustomMessage(int customMessageType)
//      : IMessage(MessageType_CustomMessage),
//        customMessageType_(customMessageType)
//    {}
//    virtual ~ICustomMessage() {}

//    virtual int GetType() const {return customMessageType_;}
//  };


//  // This is referencing an object and member function that can be notified
//  // by an IObservable.  The object must derive from IO
//  // The member functions must be of type "void Function(const IMessage& message)" or reference a derived class of IMessage
//  class ICallable : public boost::noncopyable
//  {
//  public:
//    virtual ~ICallable()
//    {
//    }

//    virtual void Apply(const IMessage& message) = 0;

//    virtual MessageType GetMessageType() const = 0;
//    virtual IObserver* GetObserver() const = 0;
//  };

//  template <typename TObserver,
//            typename TMessage>
//  class Callable : public ICallable
//  {
//  private:
//    typedef void (TObserver::* MemberFunction) (const TMessage&);

//    TObserver&      observer_;
//    MemberFunction  function_;

//  public:
//    Callable(TObserver& observer,
//             MemberFunction function) :
//      observer_(observer),
//      function_(function)
//    {
//    }

//    void ApplyInternal(const TMessage& message)
//    {
//      (observer_.*function_) (message);
//    }

//    virtual void Apply(const IMessage& message)
//    {
//      ApplyInternal(dynamic_cast<const TMessage&>(message));
//    }

//    virtual MessageType GetMessageType() const
//    {
//      return static_cast<MessageType>(TMessage::Type);
//    }

//    virtual IObserver* GetObserver() const
//    {
//      return &observer_;
//    }
//  };




//  /*
//   * This is a central message broker.  It keeps track of all observers and knows
//   * when an observer is deleted.
//   * This way, it can prevent an observable to send a message to a delete observer.
//   */
//  class MessageBroker : public boost::noncopyable
//  {

//    std::set<IObserver*> activeObservers_;  // the list of observers that are currently alive (that have not been deleted)

//  public:

//    void Register(IObserver& observer)
//    {
//      activeObservers_.insert(&observer);
//    }

//    void Unregister(IObserver& observer)
//    {
//      activeObservers_.erase(&observer);
//    }

//    bool IsActive(IObserver* observer)
//    {
//      return activeObservers_.find(observer) != activeObservers_.end();
//    }
//  };


//  class Promise : public boost::noncopyable
//  {
//  protected:
//    MessageBroker&                    broker_;

//    ICallable* successCallable_;
//    ICallable* failureCallable_;

//  public:
//    Promise(MessageBroker& broker)
//      : broker_(broker),
//        successCallable_(NULL),
//        failureCallable_(NULL)
//    {
//    }

//    void Success(const IMessage& message)
//    {
//      // check the target is still alive in the broker
//      if (broker_.IsActive(successCallable_->GetObserver()))
//      {
//        successCallable_->Apply(message);
//      }
//    }

//    void Failure(const IMessage& message)
//    {
//      // check the target is still alive in the broker
//      if (broker_.IsActive(failureCallable_->GetObserver()))
//      {
//        failureCallable_->Apply(message);
//      }
//    }

//    Promise& Then(ICallable* successCallable)
//    {
//      if (successCallable_ != NULL)
//      {
//        // TODO: throw throw new "Promise may only have a single success target"
//      }
//      successCallable_ = successCallable;
//      return *this;
//    }

//    Promise& Else(ICallable* failureCallable)
//    {
//      if (failureCallable_ != NULL)
//      {
//        // TODO: throw throw new "Promise may only have a single failure target"
//      }
//      failureCallable_ = failureCallable;
//      return *this;
//    }

//  };

//  class IObserver : public boost::noncopyable
//  {
//  protected:
//    MessageBroker&                    broker_;

//  public:
//    IObserver(MessageBroker& broker)
//      : broker_(broker)
//    {
//      broker_.Register(*this);
//    }

//    virtual ~IObserver()
//    {
//      broker_.Unregister(*this);
//    }

//  };


//  class IObservable : public boost::noncopyable
//  {
//  protected:
//    MessageBroker&                     broker_;

//    typedef std::map<int, std::set<ICallable*> >   Callables;
//    Callables  callables_;
//  public:

//    IObservable(MessageBroker& broker)
//      : broker_(broker)
//    {
//    }

//    virtual ~IObservable()
//    {
//      for (Callables::const_iterator it = callables_.begin();
//           it != callables_.end(); ++it)
//      {
//        for (std::set<ICallable*>::const_iterator
//               it2 = it->second.begin(); it2 != it->second.end(); ++it2)
//        {
//          delete *it2;
//        }
//      }
//    }

//    void Register(ICallable* callable)
//    {
//      MessageType messageType = callable->GetMessageType();

//      callables_[messageType].insert(callable);
//    }

//    void EmitMessage(const IMessage& message)
//    {
//      Callables::const_iterator found = callables_.find(message.GetType());

//      if (found != callables_.end())
//      {
//        for (std::set<ICallable*>::const_iterator
//               it = found->second.begin(); it != found->second.end(); ++it)
//        {
//          if (broker_.IsActive((*it)->GetObserver()))
//          {
//            (*it)->Apply(message);
//          }
//        }
//      }
//    }

//  };


//  enum CustomMessageType
//  {
//    CustomMessageType_First = MessageType_LastGenericStoneMessage + 1,

//    CustomMessageType_Completed,
//    CustomMessageType_Increment
//  };

  using namespace OrthancStone;

  class MyObservable : public IObservable
  {
  public:
    struct MyCustomMessage: public BaseMessage<MessageType_Test1>
    {
      int payload_;

      MyCustomMessage(int payload)
        : BaseMessage(),
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

    void HandleCompletedMessage(const MyObservable::MyCustomMessage& message)
    {
      testCounter += message.payload_;
    }

  };


  class MyIntermediate : public IObserver, public IObservable
  {
    IObservable& observedObject_;
  public:
    MyIntermediate(MessageBroker& broker, IObservable& observedObject)
      : IObserver(broker),
        IObservable(broker),
        observedObject_(observedObject)
    {
      observedObject_.RegisterObserverCallback(new MessageForwarder<MyObservable::MyCustomMessage>(broker, *this));
    }
  };


  class MyPromiseSource : public IObservable
  {
    Promise* currentPromise_;
  public:
    struct MyPromiseMessage: public BaseMessage<MessageType_Test1>
    {
      int increment;

      MyPromiseMessage(int increment)
        : BaseMessage(),
          increment(increment)
      {}
    };

    MyPromiseSource(MessageBroker& broker)
      : IObservable(broker),
        currentPromise_(NULL)
    {}

    Promise& StartSomethingAsync()
    {
      currentPromise_ = new Promise(broker_);
      return *currentPromise_;
    }

    void CompleteSomethingAsyncWithSuccess(int payload)
    {
      currentPromise_->Success(MyPromiseMessage(payload));
      delete currentPromise_;
    }

    void CompleteSomethingAsyncWithFailure(int payload)
    {
      currentPromise_->Failure(MyPromiseMessage(payload));
      delete currentPromise_;
    }
  };


  class MyPromiseTarget : public IObserver
  {
  public:
    MyPromiseTarget(MessageBroker& broker)
      : IObserver(broker)
    {}

    void IncrementCounter(const MyPromiseSource::MyPromiseMessage& args)
    {
      testCounter += args.increment;
    }

    void DecrementCounter(const MyPromiseSource::MyPromiseMessage& args)
    {
      testCounter -= args.increment;
    }
  };
}


TEST(MessageBroker2, TestPermanentConnectionSimpleUseCase)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyObserver    observer(broker);

  // create a permanent connection between an observable and an observer
  observable.RegisterObserverCallback(new Callable<MyObserver, MyObservable::MyCustomMessage>(observer, &MyObserver::HandleCompletedMessage));

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
  observable.RegisterObserverCallback(new Callable<MyObserver, MyObservable::MyCustomMessage>(*observer, &MyObserver::HandleCompletedMessage));

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

TEST(MessageBroker2, TestMessageForwarderSimpleUseCase)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyIntermediate intermediate(broker, observable);
  MyObserver    observer(broker);

  // let the observer observers the intermediate that is actually forwarding the messages from the observable
  intermediate.RegisterObserverCallback(new Callable<MyObserver, MyObservable::MyCustomMessage>(observer, &MyObserver::HandleCompletedMessage));

  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(12, testCounter);

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(20, testCounter);
}



TEST(MessageBroker2, TestPromiseSuccessFailure)
{
  MessageBroker broker;
  MyPromiseSource  source(broker);
  MyPromiseTarget target(broker);

  // test a successful promise
  source.StartSomethingAsync()
      .Then(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(target, &MyPromiseTarget::IncrementCounter))
      .Else(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(target, &MyPromiseTarget::DecrementCounter));

  testCounter = 0;
  source.CompleteSomethingAsyncWithSuccess(10);
  ASSERT_EQ(10, testCounter);

  // test a failing promise
  source.StartSomethingAsync()
      .Then(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(target, &MyPromiseTarget::IncrementCounter))
      .Else(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(target, &MyPromiseTarget::DecrementCounter));

  testCounter = 0;
  source.CompleteSomethingAsyncWithFailure(15);
  ASSERT_EQ(-15, testCounter);
}

TEST(MessageBroker2, TestPromiseDeleteTarget)
{
  MessageBroker broker;
  MyPromiseSource source(broker);
  MyPromiseTarget* target = new MyPromiseTarget(broker);

  // create the promise
  source.StartSomethingAsync()
      .Then(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(*target, &MyPromiseTarget::IncrementCounter))
      .Else(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(*target, &MyPromiseTarget::DecrementCounter));

  // delete the promise target
  delete target;

  // trigger the promise, make sure it does not throw and does not call the callback
  testCounter = 0;
  source.CompleteSomethingAsyncWithSuccess(10);
  ASSERT_EQ(0, testCounter);

  // test a failing promise
  source.StartSomethingAsync()
      .Then(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(*target, &MyPromiseTarget::IncrementCounter))
      .Else(new Callable<MyPromiseTarget, MyPromiseSource::MyPromiseMessage>(*target, &MyPromiseTarget::DecrementCounter));

  testCounter = 0;
  source.CompleteSomethingAsyncWithFailure(15);
  ASSERT_EQ(0, testCounter);
}

