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
