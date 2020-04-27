#pragma once

#include <string>
#include <iostream>

#include <Core/Logging.h>

namespace OrthancStoneHelpers
{
  inline void SetLogLevel(std::string logLevel)
  {
    boost::to_lower(logLevel);
    if (logLevel == "warning")
    {
      Orthanc::Logging::EnableInfoLevel(false);
      Orthanc::Logging::EnableTraceLevel(false);
    }
    else if (logLevel == "info")
    {
      Orthanc::Logging::EnableInfoLevel(true);
      Orthanc::Logging::EnableTraceLevel(false);
    }
    else if (logLevel == "trace")
    {
      Orthanc::Logging::EnableInfoLevel(true);
      Orthanc::Logging::EnableTraceLevel(true);
    }
    else
    {
      std::cerr << "Unknown log level \"" << logLevel << "\". Will use TRACE as default!";
      Orthanc::Logging::EnableInfoLevel(true);
      Orthanc::Logging::EnableTraceLevel(true);
    }
  }
}