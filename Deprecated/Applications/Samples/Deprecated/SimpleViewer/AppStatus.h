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
