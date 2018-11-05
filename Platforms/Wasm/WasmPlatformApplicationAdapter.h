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

      virtual void HandleMessageFromWeb(std::string& output, const std::string& input);
      virtual void NotifyStatusUpdateFromCppToWeb(const std::string& statusUpdateMessage);
  };
}