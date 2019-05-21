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
