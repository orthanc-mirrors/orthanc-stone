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
    // used in unit tests only
    MessageType_Test1,
    MessageType_Test2,

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

    MessageType GetType() const {return messageType_;}
  };


  class IObserver;
  class IObservable;

  /*
   * This is a central message broker.  It keeps track of all observers and knows
   * when an observer is deleted.
   * This way, it can prevent an observable to send a message to a dead observer.
   */
  class MessageBroker : public boost::noncopyable
  {

    std::set<IObserver*> activeObservers_;  // the list of observers that are currently alive (that have not been deleted)

  public:

    void Register(IObserver& observer)
    {
      activeObservers_.insert(&observer);
    }

    void Unregister(IObserver& observer)
    {
      activeObservers_.erase(&observer);
    }

    void EmitMessage(IObservable& from, std::set<IObserver*> observers, const IMessage& message);
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

    void HandleMessage_(IObservable &from, const IMessage &message)
    {

      HandleMessage(from, message);
    }

    virtual void HandleMessage(IObservable& from, const IMessage& message) = 0;


  protected:


  };

//  struct ICallableObserver
//  {
//    IObserver* observer;
//  };

//  typedef void (IObserver::*ObserverSingleMesssageHandler)(IObservable& from, const IMessage& message);

//  template <typename TObserver>
//  struct CallableObserver : public ICallableObserver
//  {
//    void (TObserver::*ptrToMemberHandler)(IObservable& from, const IMessage& message);
//  };

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

    std::map<MessageType, std::set<CallableObserver*> > callables_;
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

      // TODO check if observer is still alive and call !
      CallableObserver* callable = *(callables_[message.GetType()].begin());
      callable->f(*this, message);
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
    void Connect(MessageType messageType, IObserver& observer, boost::function<void (IObservable& from, const IMessage& message)> f)
    {
      callables_[messageType] = std::set<CallableObserver*>();
      CallableObserver* callable = new CallableObserver();
      callable->observer = &observer;
      callable->f = f;
      callables_[messageType].insert(callable);
    }
  };


  class MyObservable : public IObservable
  {
  public:
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
    virtual void HandleMessage(IObservable& from, const IMessage& message) {}
    void HandleSpecificMessage(IObservable& from, const IMessage& message)
    {
      testCounter++;
    }

  };

}

//#define STONE_CONNECT(observabe, messageType, observerPtr, observerMemberFnPtr)

TEST(MessageBroker2, Test1)
{
  MessageBroker broker;
  MyObservable  observable(broker);
  MyObserver    observer(broker);


  observable.Connect(MessageType_Test1, observer, boost::bind(&MyObserver::HandleSpecificMessage, &observer, _1, _2));
  //STONE_CONNECT(observable, MessageType_Test1, observer, &MyObserver::HandleSpecificMessage)
  observable.EmitMessage(IMessage(MessageType_Test1));

  ASSERT_EQ(1, testCounter);
}


