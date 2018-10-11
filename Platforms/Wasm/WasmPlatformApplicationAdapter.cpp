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

    void WasmPlatformApplicationAdapter::HandleMessageFromWeb(std::string& output, const std::string& input)
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
        printf("Error while handling message from web (error code = %d):\n", exc.GetErrorCode());
        printf("While interpreting input: '%s'\n", input.c_str());
      }
    }

    void WasmPlatformApplicationAdapter::NotifyStatusUpdateFromCppToWeb(const std::string& statusUpdateMessage)
    {
      printf("NotifyStatusUpdateFromCppToWeb (TODO)\n");
      UpdateStoneApplicationStatusFromCpp(statusUpdateMessage.c_str());
      printf("NotifyStatusUpdateFromCppToWeb (DONE)\n");
    }

}