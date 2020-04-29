#include "WasmPlatformApplicationAdapter.h"

#include "Framework/StoneException.h"
#include <stdio.h>
#include "Platforms/Wasm/Defaults.h"

namespace OrthancStone
{
  WasmPlatformApplicationAdapter::WasmPlatformApplicationAdapter(MessageBroker& broker, IStoneApplication& application)
  : IObserver(broker),
    application_(application)
  {
  }

  void WasmPlatformApplicationAdapter::HandleSerializedMessageFromWeb(std::string& output, const std::string& input)
  {
    try
    {
      application_.HandleSerializedMessage(input.c_str());
    }
    catch (StoneException& exc)
    {
      printf("Error while handling message from web (error code = %d):\n", exc.GetErrorCode());
      printf("While interpreting input: '%s'\n", input.c_str());
      output = std::string("ERROR : ");
    }
    catch (std::exception& exc)
    {
      printf("Error while handling message from web (error text = %s):\n", exc.what());
      printf("While interpreting input: '%s'\n", input.c_str());
      output = std::string("ERROR : ");
    }
  }

  void WasmPlatformApplicationAdapter::NotifyStatusUpdateFromCppToWebWithString(const std::string& statusUpdateMessage)
  {
    try
    {
      UpdateStoneApplicationStatusFromCppWithString(statusUpdateMessage.c_str());
    }
    catch (...)
    {
      printf("Error while handling string message to web\n");
    }
  }

  void WasmPlatformApplicationAdapter::NotifyStatusUpdateFromCppToWebWithSerializedMessage(const std::string& statusUpdateMessage)
  {
    try
    {
      UpdateStoneApplicationStatusFromCppWithSerializedMessage(statusUpdateMessage.c_str());
    }
    catch (...)
    {
      printf("Error while handling serialized message to web\n");
    }
  }

}
