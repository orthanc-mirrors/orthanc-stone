/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include "BaseCommandBuilder.h"
#include "Core/OrthancException.h"
#include <iostream>
#include "Framework/StoneException.h"

namespace OrthancStone
{
  ICommand* BaseCommandBuilder::CreateFromJson(const Json::Value& commandJson)
  {
    if (!commandJson.isObject() || !commandJson["command"].isString())
    {
      throw StoneException(ErrorCode_CommandJsonInvalidFormat);
    }

    if (commandJson["commandType"].isString() && commandJson["commandType"].asString() == "generic-no-arg-command")
    {
        printf("creating a simple command\n");
        return new GenericNoArgCommand(commandJson["command"].asString().c_str());
    }
    else if (commandJson["commandType"].isString() && commandJson["commandType"].asString() == "generic-one-string-arg-command")
    {
        printf("creating a simple command\n");
        return new GenericNoArgCommand(commandJson["command"].asString().c_str());
    }

    return NULL;
  }

}