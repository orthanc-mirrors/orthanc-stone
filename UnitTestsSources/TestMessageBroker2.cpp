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

  using namespace OrthancStone;


  enum CustomMessageType
  {
    CustomMessageType_First = MessageType_CustomMessage + 1,

    CustomMessageType_Completed,
    CustomMessageType_Increment
  };


  class MyObservable : public IObservable
  {
  public:
    struct MyCustomMessage: public BaseMessage<CustomMessageType_Completed>
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
      currentPromise_ = new Promise(GetBroker());
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

TEST(MessageBroker2, TestMessageForwarderDeleteIntermediate)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyIntermediate* intermediate = new MyIntermediate(broker, observable);
  MyObserver    observer(broker);

  // let the observer observers the intermediate that is actually forwarding the messages from the observable
  intermediate->RegisterObserverCallback(new Callable<MyObserver, MyObservable::MyCustomMessage>(observer, &MyObserver::HandleCompletedMessage));

  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(12, testCounter);

  delete intermediate;

  observable.EmitMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(12, testCounter);
}

TEST(MessageBroker2, TestCustomMessage)
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

#if __cplusplus >= 201103L

#include <functional>

namespace OrthancStone {

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


}

TEST(MessageBroker2, TestLambdaSimpleUseCase)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyObserver*   observer = new MyObserver(broker);

  // create a permanent connection between an observable and an observer
  observable.RegisterObserverCallback(new LambdaCallable<MyObservable::MyCustomMessage>(*observer, [&](const MyObservable::MyCustomMessage& message) {testCounter += 2 * message.payload_;}));

  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(24, testCounter);

  // delete the observer and check that the callback is not called anymore
  delete observer;

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(0, testCounter);
}

namespace {
  class MyObserverWithLambda : public IObserver {
  private:
    int multiplier_;  // this is a private variable we want to access in a lambda

  public:
    MyObserverWithLambda(MessageBroker& broker, int multiplier, MyObservable& observable)
      : IObserver(broker),
        multiplier_(multiplier)
    {
      // register a callable to a lambda that access private members
      observable.RegisterObserverCallback(new LambdaCallable<MyObservable::MyCustomMessage>(*this, [this](const MyObservable::MyCustomMessage& message) {
        testCounter += multiplier_ * message.payload_;
      }));

    }
  };
}

TEST(MessageBroker2, TestLambdaCaptureThisAndAccessPrivateMembers)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyObserverWithLambda*   observer = new MyObserverWithLambda(broker, 3, observable);

  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(36, testCounter);

  // delete the observer and check that the callback is not called anymore
  delete observer;

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.EmitMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(0, testCounter);
}

#endif // C++ 11