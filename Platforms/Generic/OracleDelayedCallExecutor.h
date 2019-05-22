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

#include "../../Framework/Deprecated/Toolbox/IDelayedCallExecutor.h"
#include "Oracle.h"
#include "../../Applications/Generic/NativeStoneApplicationContext.h"
#include "DelayedCallCommand.h"

namespace Deprecated
{
  // The OracleTimeout executes callbacks after a delay.
  class OracleDelayedCallExecutor : public IDelayedCallExecutor
  {
  private:
    Oracle&                        oracle_;
    OrthancStone::NativeStoneApplicationContext& context_;

  public:
    OracleDelayedCallExecutor(OrthancStone::MessageBroker& broker,
                              Oracle& oracle,
                              OrthancStone::NativeStoneApplicationContext& context) :
      IDelayedCallExecutor(broker),
      oracle_(oracle),
      context_(context)
    {
    }

    virtual void Schedule(OrthancStone::MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,
                          unsigned int timeoutInMs = 1000)
    {
      oracle_.Submit(new DelayedCallCommand(broker_, callback, timeoutInMs, NULL, context_));
    }
  };
}
