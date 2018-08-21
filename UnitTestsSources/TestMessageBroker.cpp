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

#include "../Framework/Messages/MessageBroker.h"
#include "../Framework/Messages/IMessage.h"
#include "../Framework/Messages/IObservable.h"
#include "../Framework/Messages/IObserver.h"
#include "../Framework/StoneEnumerations.h"


static int globalCounter = 0;
class MyObserver : public OrthancStone::IObserver
{

public:
  MyObserver(OrthancStone::MessageBroker& broker)
    : OrthancStone::IObserver(broker)
  {}


  void HandleMessage(const OrthancStone::IObservable& from, const OrthancStone::IMessage& message) {
    if (message.GetType() == OrthancStone::MessageType_Generic) {
      globalCounter++;
    }

  }

};


TEST(MessageBroker, NormalUsage)
{
  OrthancStone::MessageBroker broker;
  OrthancStone::IObservable observable(broker);

  globalCounter = 0;

  OrthancStone::IMessage genericMessage(OrthancStone::MessageType_Generic);

  // no observers have been registered -> nothing shall happen
  observable.EmitMessage(genericMessage);

  ASSERT_EQ(0, globalCounter);

  // register an observer, check it is called
  MyObserver observer(broker);
  observable.RegisterObserver(observer);

  observable.EmitMessage(genericMessage);

  ASSERT_EQ(1, globalCounter);

  // check the observer is not called when another message is issued
  OrthancStone::IMessage wrongMessage(OrthancStone::MessageType_GeometryReady);
  // no observers have been registered
  observable.EmitMessage(wrongMessage);

  ASSERT_EQ(1, globalCounter);

  // unregister the observer, make sure nothing happens afterwards
  observable.UnregisterObserver(observer);
  observable.EmitMessage(genericMessage);
  ASSERT_EQ(1, globalCounter);
}

TEST(MessageBroker, DeleteObserverWhileRegistered)
{
  OrthancStone::MessageBroker broker;
  OrthancStone::IObservable observable(broker);

  globalCounter = 0;

  OrthancStone::IMessage genericMessage(OrthancStone::MessageType_Generic);

  {
    // register an observer, check it is called
    MyObserver observer(broker);
    observable.RegisterObserver(observer);

    observable.EmitMessage(genericMessage);

    ASSERT_EQ(1, globalCounter);
  }

  // at this point, the observer has been deleted, the handle shall not be called again (and it shall not crash !)
  observable.EmitMessage(genericMessage);

  ASSERT_EQ(1, globalCounter);
}
