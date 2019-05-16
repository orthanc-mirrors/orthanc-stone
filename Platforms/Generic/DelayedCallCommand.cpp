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


#include "DelayedCallCommand.h"
#include "boost/thread/thread.hpp"

#include <iostream>

namespace OrthancStone
{
  DelayedCallCommand::DelayedCallCommand(MessageBroker& broker,
                                         MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,  // takes ownership
                                         unsigned int timeoutInMs,
                                         Orthanc::IDynamicObject* payload /* takes ownership */,
                                         NativeStoneApplicationContext& context
                                         ) :
    IObservable(broker),
    callback_(callback),
    payload_(payload),
    context_(context),
    expirationTimePoint_(boost::posix_time::microsec_clock::local_time() + boost::posix_time::milliseconds(timeoutInMs)),
    timeoutInMs_(timeoutInMs)
  {
  }


  void DelayedCallCommand::Execute()
  {
    while (boost::posix_time::microsec_clock::local_time() < expirationTimePoint_)
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }
  }

  void DelayedCallCommand::Commit()
  {
    // We want to make sure that, i.e, the UpdateThread is not
    // triggered while we are updating the "model" with the result of
    // an OracleCommand
    NativeStoneApplicationContext::GlobalMutexLocker lock(context_);

    if (callback_.get() != NULL)
    {
      IDelayedCallExecutor::TimeoutMessage message; // TODO: add payload
      callback_->Apply(message);
    }
  }
}
