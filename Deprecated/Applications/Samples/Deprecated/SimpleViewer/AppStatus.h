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


#pragma once

#include <string>


namespace SimpleViewer
{
  struct AppStatus
  {
    std::string patientId;
    std::string studyDescription;
    std::string currentInstanceIdInMainViewport;
    // note: if you add members here, update the serialization code below and deserialization in simple-viewer.ts -> onAppStatusUpdated()


    AppStatus()
    {
    }

    void ToJson(Json::Value &output) const
    {
      output["patientId"] = patientId;
      output["studyDescription"] = studyDescription;
      output["currentInstanceIdInMainViewport"] = currentInstanceIdInMainViewport;
    }
  };
}
