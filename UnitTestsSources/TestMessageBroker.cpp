///**
// * Stone of Orthanc
// * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
// * Department, University Hospital of Liege, Belgium
// * Copyright (C) 2017-2019 Osimis S.A., Belgium
// *
// * This program is free software: you can redistribute it and/or
// * modify it under the terms of the GNU Affero General Public License
// * as published by the Free Software Foundation, either version 3 of
// * the License, or (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful, but
// * WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// * Affero General Public License for more details.
// *
// * You should have received a copy of the GNU Affero General Public License
// * along with this program. If not, see <http://www.gnu.org/licenses/>.
// **/


//#include "gtest/gtest.h"

//#include "../Framework/Messages/MessageBroker.h"
//#include "../Framework/Messages/IMessage.h"
//#include "../Framework/Messages/IObservable.h"
//#include "../Framework/Messages/IObserver.h"
//#include "../Framework/StoneEnumerations.h"


//static int test1Counter = 0;
//static int test2Counter = 0;
//class MyFullObserver : public OrthancStone::IObserver
//{

//public:
//  MyFullObserver(OrthancStone::MessageBroker& broker)
//    : OrthancStone::IObserver(broker)
//  {
////    DeclareHandledMessage(OrthancStone::MessageType_Test1);
////    DeclareIgnoredMessage(OrthancStone::MessageType_Test2);
//  }


//  void HandleMessage(OrthancStone::IObservable& from, const OrthancStone::IMessage& message) {
//    switch (message.GetType())
//    {
//    case OrthancStone::MessageType_Test1:
//      test1Counter++;
//      break;
//    case OrthancStone::MessageType_Test2:
//      test2Counter++;
//      break;
//    default:
//      throw OrthancStone::MessageNotDeclaredException(message.GetType());
//    }
//  }

//};

//class MyPartialObserver : public OrthancStone::IObserver
//{

//public:
//  MyPartialObserver(OrthancStone::MessageBroker& broker)
//    : OrthancStone::IObserver(broker)
//  {
////    DeclareHandledMessage(OrthancStone::MessageType_Test1);
//    // don't declare Test2 on purpose
//  }


//  void HandleMessage(OrthancStone::IObservable& from, const OrthancStone::IMessage& message) {
//    switch (message.GetType())
//    {
//    case OrthancStone::MessageType_Test1:
//      test1Counter++;
//      break;
//    case OrthancStone::MessageType_Test2:
//      test2Counter++;
//      break;
//    default:
//      throw OrthancStone::MessageNotDeclaredException(message.GetType());
//    }
//  }

//};


//class MyObservable : public OrthancStone::IObservable
//{

//public:
//  MyObservable(OrthancStone::MessageBroker& broker)
//    : OrthancStone::IObservable(broker)
//  {
//    DeclareEmittableMessage(OrthancStone::MessageType_Test1);
//    DeclareEmittableMessage(OrthancStone::MessageType_Test2);
//  }

//};


//TEST(MessageBroker, NormalUsage)
//{
//  OrthancStone::MessageBroker broker;
//  MyObservable observable(broker);

//  test1Counter = 0;

//  // no observers have been registered -> nothing shall happen
//  observable.EmitMessage(OrthancStone::IMessage(OrthancStone::MessageType_Test1));

//  ASSERT_EQ(0, test1Counter);

//  // register an observer, check it is called
//  MyFullObserver fullObserver(broker);
//  ASSERT_NO_THROW(observable.RegisterObserver(fullObserver));

//  observable.EmitMessage(OrthancStone::IMessage(OrthancStone::MessageType_Test1));

//  ASSERT_EQ(1, test1Counter);

//  // register an invalid observer, check it raises an exception
//  MyPartialObserver partialObserver(broker);
//  ASSERT_THROW(observable.RegisterObserver(partialObserver), OrthancStone::MessageNotDeclaredException);

//  // check an exception is thrown when the observable emits an undeclared message
//  ASSERT_THROW(observable.EmitMessage(OrthancStone::IMessage(OrthancStone::MessageType_VolumeSlicer_GeometryReady)), OrthancStone::MessageNotDeclaredException);

//  // unregister the observer, make sure nothing happens afterwards
//  observable.UnregisterObserver(fullObserver);
//  observable.EmitMessage(OrthancStone::IMessage(OrthancStone::MessageType_Test1));
//  ASSERT_EQ(1, test1Counter);
//}

//TEST(MessageBroker, DeleteObserverWhileRegistered)
//{
//  OrthancStone::MessageBroker broker;
//  MyObservable observable(broker);

//  test1Counter = 0;

//  {
//    // register an observer, check it is called
//    MyFullObserver observer(broker);
//    observable.RegisterObserver(observer);

//    observable.EmitMessage(OrthancStone::IMessage(OrthancStone::MessageType_Test1));

//    ASSERT_EQ(1, test1Counter);
//  }

//  // at this point, the observer has been deleted, the handle shall not be called again (and it shall not crash !)
//  observable.EmitMessage(OrthancStone::IMessage(OrthancStone::MessageType_Test1));

//  ASSERT_EQ(1, test1Counter);
//}
