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


#pragma once

#include "../../Framework/Messages/IObserver.h"
#include "../../Framework/Messages/ICallable.h"

#include <Core/IDynamicObject.h>
#include <Core/Logging.h>

#include <string>
#include <map>

namespace OrthancStone
{
  // The IDelayedCall executes a callback after a delay (equivalent to timeout() function in javascript).
  class IDelayedCallExecutor : public boost::noncopyable
  {
  protected:
    MessageBroker& broker_;
    
  public:

    typedef NoPayloadMessage<MessageType_Timeout> TimeoutMessage;

    IDelayedCallExecutor(MessageBroker& broker) :
      broker_(broker)
    {
    }

    
    virtual ~IDelayedCallExecutor()
    {
    }

    
    virtual void Schedule(MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,
                         unsigned int timeoutInMs = 1000) = 0;
  };
}