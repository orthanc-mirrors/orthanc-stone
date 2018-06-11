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


#include "IBasicApplication.h"

#include "../Framework/Toolbox/MessagingToolbox.h"
#include "Sdl/SdlEngine.h"

#include <Core/Logging.h>
#include <Core/HttpClient.h>
#include <Plugins/Samples/Common/OrthancHttpConnection.h>

namespace OrthancStone
{
    void IBasicApplication::DeclareStringStartupOption(const std::string& name, const std::string& defaultValue, const std::string& helpText) {
        StartupOption option;
        option.name = name;
        option.defaultValue = defaultValue;
        option.helpText = helpText = helpText;
        option.type = StartupOption::string;

        startupOptions_.push_back(option);
    }

    void IBasicApplication::DeclareBoolStartupOption(const std::string& name, bool defaultValue, const std::string& helpText) {
        StartupOption option;
        option.name = name;
        option.defaultValue = (defaultValue ? "true" : "false");
        option.helpText = helpText = helpText;
        option.type = StartupOption::boolean;

        startupOptions_.push_back(option);
    }

    void IBasicApplication::DeclareIntegerStartupOption(const std::string& name, const int& defaultValue, const std::string& helpText) {
        StartupOption option;
        option.name = name;
        option.defaultValue = std::to_string(defaultValue);
        option.helpText = helpText = helpText;
        option.type = StartupOption::integer;

        startupOptions_.push_back(option);
    }

}
