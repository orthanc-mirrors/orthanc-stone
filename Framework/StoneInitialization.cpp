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
#include <Core/Logging.h>

#if !defined(ORTHANC_ENABLE_SDL)
#  error Macro ORTHANC_ENABLE_SDL must be defined
#endif


#if ORTHANC_ENABLE_SDL == 1
#  include "../Applications/Sdl/SdlWindow.h"
#endif

#if ORTHANC_ENABLE_OPENGL == 1
#  include "GL/glew.h"
#endif

namespace OrthancStone
{
  void StoneInitialize()
  {
    Orthanc::Logging::Initialize();

#if 0
#if ORTHANC_ENABLE_OPENGL == 1
    glEnable(GL_DEBUG_OUTPUT);
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      const char* error =
        reinterpret_cast<const char*>(glewGetErrorString(err));
      std::stringstream message;
      message << "Using GLEW version " << reinterpret_cast<const char*>(
        glewGetString(GLEW_VERSION));
      LOG(INFO) << "Using GLEW version " << message.str();
    }
#endif
#endif

#if ORTHANC_ENABLE_SDL == 1
    OrthancStone::SdlWindow::GlobalInitialize();
#endif
  }

  void StoneFinalize()
  {
#if ORTHANC_ENABLE_SDL == 1
    OrthancStone::SdlWindow::GlobalFinalize();
#endif
    
    Orthanc::Logging::Finalize();
  }
}
