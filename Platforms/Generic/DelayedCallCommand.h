/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include "../../Framework/Deprecated/Toolbox/IDelayedCallExecutor.h"
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Messages/ICallable.h"
#include "../../Applications/Generic/NativeStoneApplicationContext.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace Deprecated
{
  class DelayedCallCommand : public IOracleCommand, OrthancStone::IObservable
  {
  protected:
    std::unique_ptr<OrthancStone::MessageHandler<IDelayedCallExecutor::TimeoutMessage> >  callback_;
    std::unique_ptr<Orthanc::IDynamicObject>  payload_;
    OrthancStone::NativeStoneApplicationContext&          context_;
    boost::posix_time::ptime                expirationTimePoint_;
    unsigned int                            timeoutInMs_;

  public:
    DelayedCallCommand(OrthancStone::MessageBroker& broker,
                       OrthancStone::MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,  // takes ownership
                       unsigned int timeoutInMs,
                       Orthanc::IDynamicObject* payload /* takes ownership */,
                       OrthancStone::NativeStoneApplicationContext& context
                       );

    virtual void Execute();

    virtual void Commit();
  };

}
