#pragma once

#include <string>


namespace CtPetDoseStructFusion
{
  struct AppStatus
  {
    std::string patientId;
    std::string studyDescription;
    std::string currentSeriesIdInMainViewport;
    // note: if you add members here, update the serialization code below and deserialization in ct-dose-struct-fusion.ts -> onAppStatusUpdated()


    AppStatus()
    {
    }

    void ToJson(Json::Value &output) const
    {
      output["patientId"] = patientId;
      output["studyDescription"] = studyDescription;
      output["currentSeriesIdInMainViewport"] = currentSeriesIdInMainViewport;
    }
  };
}
