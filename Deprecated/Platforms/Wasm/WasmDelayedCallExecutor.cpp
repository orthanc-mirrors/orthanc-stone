#include "WasmDelayedCallExecutor.h"
#include "json/value.h"
#include "json/writer.h"
#include <emscripten/emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

  extern void WasmDelayedCallExecutor_Schedule(void* callable,
                                      unsigned int timeoutInMs
                                      /*void* payload*/);

  void EMSCRIPTEN_KEEPALIVE WasmDelayedCallExecutor_ExecuteCallback(void* callable
                                                       //void* payload
                                                       )
  {
    if (callable == NULL)
    {
      throw;
    }
    else
    {
      reinterpret_cast<OrthancStone::MessageHandler<Deprecated::IDelayedCallExecutor::TimeoutMessage>*>(callable)->
        Apply(Deprecated::IDelayedCallExecutor::TimeoutMessage()); // uri, reinterpret_cast<Orthanc::IDynamicObject*>(payload)));
    }
  }


#ifdef __cplusplus
}
#endif



namespace Deprecated
{
  OrthancStone::MessageBroker* WasmDelayedCallExecutor::broker_ = NULL;


  void WasmDelayedCallExecutor::Schedule(OrthancStone::MessageHandler<IDelayedCallExecutor::TimeoutMessage>* callback,
                         unsigned int timeoutInMs)
  {
    WasmDelayedCallExecutor_Schedule(callback, timeoutInMs);
  }
}
