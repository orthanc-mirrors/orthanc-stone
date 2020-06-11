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

#include "../../Framework/Deprecated/Toolbox/IDelayedCallExecutor.h"
#include <Core/OrthancException.h>

namespace Deprecated
{
  class WasmDelayedCallExecutor : public IDelayedCallExecutor
  {
  private:
    static OrthancStone::MessageBroker* broker_;

    // Private constructor => Singleton design pattern
    WasmDelayedCallExecutor(OrthancStone::MessageBroker& broker) :
      IDelayedCallExecutor(broker)
    {
    }

  public:
    static WasmDelayedCallExecutor& GetInstance()
    {
      if (broker_ == NULL)
      {
        printf("WasmDelayedCallExecutor::GetInstance(): broker not initialized\n");
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      static WasmDelayedCallExecutor instance(*broker_);
      return instance;
    }

    static void SetBroker(OrthancStone::MessageBroker& broker)
    {
      broker_ = &broker;
    }

    virtual void Schedule(OrthancStone::MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,
                         unsigned int timeoutInMs = 1000);

  };
}
