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

#include <Core/OrthancException.h>

#if !defined(ORTHANC_ENABLE_SDL)
#  error Macro ORTHANC_ENABLE_SDL must be defined
#endif

#if ORTHANC_ENABLE_SDL == 1
#  include "Viewport/SdlWindow.h"
#endif

#if ORTHANC_ENABLE_CURL == 1
#include <Core/HttpClient.h>
#endif

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

#if ORTHANC_ENABLE_SDL == 1
    OrthancStone::SdlWindow::GlobalInitialize();
#endif

#if ORTHANC_ENABLE_CURL == 1
    Orthanc::HttpClient::GlobalInitialize();
#endif
  }

  void StoneFinalize()
  {
#if ORTHANC_ENABLE_SDL == 1
    OrthancStone::SdlWindow::GlobalFinalize();
#endif
    
#if ORTHANC_ENABLE_CURL == 1
    Orthanc::HttpClient::GlobalFinalize();
#endif

    Orthanc::Logging::Finalize();
  }
}
