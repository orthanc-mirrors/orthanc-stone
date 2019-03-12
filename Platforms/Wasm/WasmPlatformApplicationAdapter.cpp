#include "WasmPlatformApplicationAdapter.h"

#include "Framework/Toolbox/MessagingToolbox.h"
#include "Framework/StoneException.h"
#include <Applications/Commands/BaseCommandBuilder.h>
#include <stdio.h>
#include "Platforms/Wasm/Defaults.h"

namespace OrthancStone
{
  WasmPlatformApplicationAdapter::WasmPlatformApplicationAdapter(MessageBroker& broker, IStoneApplication& application)
  : IObserver(broker),
    application_(application)
  {
  }

  void WasmPlatformApplicationAdapter::HandleCommandFromWeb(std::string& output, const std::string& input)
  {
    try
    {
      Json::Value inputJson;
      // if the message is a command, build it and execute it
      if (MessagingToolbox::ParseJson(inputJson, input.c_str(), input.size()))
      {
          std::unique_ptr<ICommand> command(application_.GetCommandBuilder().CreateFromJson(inputJson));
          if (command.get() == NULL) 
            printf("Could not parse command: '%s'\n", input.c_str());
          else
            application_.ExecuteCommand(*command);
      } 
    }
    catch (StoneException& exc)
    {
      printf("Error while handling command from web (error code = %d):\n", exc.GetErrorCode());
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