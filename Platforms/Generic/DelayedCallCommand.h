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

#include "IOracleCommand.h"

#include "../../Framework/Toolbox/IDelayedCallExecutor.h"
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Messages/ICallable.h"
#include "../../Applications/Generic/NativeStoneApplicationContext.h"
#include <boost/chrono.hpp>

namespace OrthancStone
{
  class DelayedCallCommand : public IOracleCommand, IObservable
  {
  protected:
    std::auto_ptr<MessageHandler<IDelayedCallExecutor::TimeoutMessage> >  callback_;
    std::auto_ptr<Orthanc::IDynamicObject>  payload_;
    NativeStoneApplicationContext&          context_;
    boost::chrono::system_clock::time_point expirationTimePoint_;
    unsigned int                            timeoutInMs_;

  public:
    DelayedCallCommand(MessageBroker& broker,
                       MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,  // takes ownership
                       unsigned int timeoutInMs,
                       Orthanc::IDynamicObject* payload /* takes ownership */,
                       NativeStoneApplicationContext& context
                       );

    virtual void Execute();

    virtual void Commit();
  };

}