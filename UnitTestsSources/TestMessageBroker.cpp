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

#include "Framework/Messages/IObservable.h"
#include "Framework/Messages/ObserverBase.h"


int testCounter = 0;
namespace {

  using namespace OrthancStone;


  class MyObservable : public IObservable
  {
  public:
    struct MyCustomMessage : public IMessage
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

      int payload_;

      MyCustomMessage(int payload) :
        payload_(payload)
      {
      }
    };
  };

  class MyObserver : public ObserverBase<MyObserver>
  {
  public:
    void HandleCompletedMessage(const MyObservable::MyCustomMessage& message)
    {
      testCounter += message.payload_;
    }
  };
}


TEST(MessageBroker, TestPermanentConnectionSimpleUseCase)
{
  MyObservable  observable;
  boost::shared_ptr<MyObserver>  observer(new MyObserver);

  // create a permanent connection between an observable and an observer
  observer->Register<MyObservable::MyCustomMessage>(observable, &MyObserver::HandleCompletedMessage);

  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(12, testCounter);

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(20, testCounter);

  // Unregister the observer; make sure it's not called anymore
  observer.reset();
  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(0, testCounter);
}

TEST(MessageBroker, TestPermanentConnectionDeleteObserver)
{
  MyObservable  observable;
  boost::shared_ptr<MyObserver>  observer(new MyObserver);

  // create a permanent connection between an observable and an observer
  observer->Register<MyObservable::MyCustomMessage>(observable, &MyObserver::HandleCompletedMessage);

  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(12, testCounter);

  // delete the observer and check that the callback is not called anymore
  observer.reset();

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(0, testCounter);
}



#if 0 /* __cplusplus >= 201103L*/

TEST(MessageBroker, TestLambdaSimpleUseCase)
{
  MyObservable  observable;
  MyObserver*   observer(new MyObserver);

  // create a permanent connection between an observable and an observer
  observable.RegisterObserverCallback(new LambdaCallable<MyObservable::MyCustomMessage>(*observer, [&](const MyObservable::MyCustomMessage& message) {testCounter += 2 * message.payload_;}));

  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(24, testCounter);

  // delete the observer and check that the callback is not called anymore
  delete observer;

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(0, testCounter);
}

namespace {
  class MyObserverWithLambda : public IObserver {
  private:
    int multiplier_;  // this is a private variable we want to access in a lambda

  public:
    MyObserverWithLambda(int multiplier, MyObservable& observable)
      : multiplier_(multiplier)
    {
      // register a callable to a lambda that access private members
      observable.RegisterObserverCallback(new LambdaCallable<MyObservable::MyCustomMessage>(*this, [this](const MyObservable::MyCustomMessage& message) {
        testCounter += multiplier_ * message.payload_;
      }));

    }
  };
}

TEST(MessageBroker, TestLambdaCaptureThisAndAccessPrivateMembers)
{
  MyObservable  observable;
  MyObserverWithLambda*   observer = new MyObserverWithLambda(3, observable);

  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(12));
  ASSERT_EQ(36, testCounter);

  // delete the observer and check that the callback is not called anymore
  delete observer;

  // the connection is permanent; if we emit the same message again, the observer will be notified again
  testCounter = 0;
  observable.BroadcastMessage(MyObservable::MyCustomMessage(20));
  ASSERT_EQ(0, testCounter);
}

#endif // C++ 11
