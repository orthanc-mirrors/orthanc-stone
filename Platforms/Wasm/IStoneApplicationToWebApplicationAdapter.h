#pragma once

#include <string>

namespace OrthancStone
{
  class IStoneApplicationToWebApplicationAdapter
  {
    public:
      virtual void HandleMessageFromWeb(std::string& output, const std::string& input) = 0;
      virtual void NotifyStatusUpdateFromCppToWeb(const std::string& statusUpdateMessage) = 0;
  };
}