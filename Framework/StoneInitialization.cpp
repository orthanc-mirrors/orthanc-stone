/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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


#include "StoneInitialization.h"

#if !defined(ORTHANC_ENABLE_SDL)
#  error Macro ORTHANC_ENABLE_SDL must be defined
#endif

#if !defined(ORTHANC_ENABLE_QT)
#  error Macro ORTHANC_ENABLE_QT must be defined
#endif

#if !defined(ORTHANC_ENABLE_SSL)
#  error Macro ORTHANC_ENABLE_SSL must be defined
#endif

#if !defined(ORTHANC_ENABLE_CURL)
#  error Macro ORTHANC_ENABLE_CURL must be defined
#endif

#if !defined(ORTHANC_ENABLE_DCMTK)
#  error Macro ORTHANC_ENABLE_DCMTK must be defined
#  if !defined(DCMTK_VERSION_NUMBER)
#    error Macro DCMTK_VERSION_NUMBER must be defined
#  endif
#endif

#if ORTHANC_ENABLE_SDL == 1
#  include "Viewport/SdlWindow.h"
#endif

#if ORTHANC_ENABLE_QT == 1
#  include <QCoreApplication>
#endif

#if ORTHANC_ENABLE_CURL == 1
#  include <Core/HttpClient.h>
#endif

#if ORTHANC_ENABLE_DCMTK == 1
#  include <Core/DicomParsing/FromDcmtkBridge.h>
#endif

#include "Toolbox/LinearAlgebra.h"

#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

#include <locale>


namespace OrthancStone
{
#if ORTHANC_ENABLE_LOGGING_PLUGIN == 1
  void StoneInitialize(OrthancPluginContext* context)
#else
  void StoneInitialize()
#endif
  {
#if ORTHANC_ENABLE_LOGGING_PLUGIN == 1
    Orthanc::Logging::Initialize(context);
#else
    Orthanc::Logging::Initialize();
#endif

#if ORTHANC_ENABLE_SSL == 1
    // Must be before curl
    Orthanc::Toolbox::InitializeOpenSsl();
#endif

#if ORTHANC_ENABLE_CURL == 1
    Orthanc::HttpClient::GlobalInitialize();
#  if ORTHANC_ENABLE_SSL == 1
    Orthanc::HttpClient::ConfigureSsl(false, "");
#  endif
#endif

#if ORTHANC_ENABLE_DCMTK == 1
    Orthanc::FromDcmtkBridge::InitializeDictionary(true);
    Orthanc::FromDcmtkBridge::InitializeCodecs();
#  if DCMTK_VERSION_NUMBER <= 360
    OFLog::configure(OFLogger::FATAL_LOG_LEVEL);
#  else
    OFLog::configure(OFLogger::OFF_LOG_LEVEL);
#  endif
#endif

    /**
     * This call is necessary to make "boost::lexical_cast<>" work in
     * a consistent way in the presence of "double" or "float", and of
     * a numeric locale that replaces dot (".") by comma (",") as the
     * decimal separator.
     * https://stackoverflow.com/a/18981514/881731
     **/
    std::locale::global(std::locale::classic());

    {
      // Run-time checks of locale settings, to be run after Qt has
      // been initialized, as Qt changes locale settings

#if ORTHANC_ENABLE_QT == 1
      if (QCoreApplication::instance() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                        "Qt must be initialized before Stone");
      }
#endif
      
      {
        OrthancStone::Vector v;
        if (!OrthancStone::LinearAlgebra::ParseVector(v, "1.3671875\\-1.3671875") ||
            v.size() != 2 ||
            !OrthancStone::LinearAlgebra::IsNear(1.3671875f, v[0]) ||
            !OrthancStone::LinearAlgebra::IsNear(-1.3671875f, v[1]))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Error in the locale settings, giving up");
        }
      }

      {
        Json::Value dicomweb = Json::objectValue;
        dicomweb["00280030"] = Json::objectValue;
        dicomweb["00280030"]["vr"] = "DS";
        dicomweb["00280030"]["Value"] = Json::arrayValue;
        dicomweb["00280030"]["Value"].append(1.2f);
        dicomweb["00280030"]["Value"].append(-1.5f);

        Orthanc::DicomMap source;
        source.FromDicomWeb(dicomweb);

        std::string s;
        OrthancStone::Vector v;
        if (!source.LookupStringValue(s, Orthanc::DICOM_TAG_PIXEL_SPACING, false) ||
            !OrthancStone::LinearAlgebra::ParseVector(v, s) ||
            v.size() != 2 ||
            !OrthancStone::LinearAlgebra::IsNear(1.2f, v[0]) ||
            !OrthancStone::LinearAlgebra::IsNear(-1.5f, v[1]))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Error in the locale settings, giving up");
        }
      }
    }

#if ORTHANC_ENABLE_SDL == 1
    OrthancStone::SdlWindow::GlobalInitialize();
#endif
  }
  

  void StoneFinalize()
  {
#if ORTHANC_ENABLE_SDL == 1
    OrthancStone::SdlWindow::GlobalFinalize();
#endif
    
#if ORTHANC_ENABLE_DCMTK == 1
    Orthanc::FromDcmtkBridge::FinalizeCodecs();
#endif

#if ORTHANC_ENABLE_CURL == 1
    Orthanc::HttpClient::GlobalFinalize();
#endif

#if ORTHANC_ENABLE_SSL == 1
    Orthanc::Toolbox::FinalizeOpenSsl();
#endif

    Orthanc::Logging::Finalize();
  }
}
