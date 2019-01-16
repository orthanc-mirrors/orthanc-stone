#pragma once

#include <Framework/Toolbox/IDelayedCallExecutor.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  class WasmDelayedCallExecutor : public IDelayedCallExecutor
  {
  private:
    static MessageBroker* broker_;

    // Private constructor => Singleton design pattern
    WasmDelayedCallExecutor(MessageBroker& broker) :
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

    static void SetBroker(MessageBroker& broker)
    {
      broker_ = &broker;
    }

    virtual void Schedule(MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,
                         unsigned int timeoutInMs = 1000);

  };
}
