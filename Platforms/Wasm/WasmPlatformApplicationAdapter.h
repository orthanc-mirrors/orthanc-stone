#pragma once

#include <string>
#include <Framework/Messages/IObserver.h>
#include <Applications/IStoneApplication.h>

namespace OrthancStone
{
  class WasmPlatformApplicationAdapter : public IObserver
  {
      IStoneApplication&  application_;
    public:
      WasmPlatformApplicationAdapter(MessageBroker& broker, IStoneApplication& application);

      virtual void HandleSerializedMessageFromWeb(std::string& output, const std::string& input);
      virtual void HandleCommandFromWeb(std::string& output, const std::string& input);
      virtual void NotifyStatusUpdateFromCppToWebWithString(const std::string& statusUpdateMessage);
      virtual void NotifyStatusUpdateFromCppToWebWithSerializedMessage(const std::string& statusUpdateMessage);
  };
}