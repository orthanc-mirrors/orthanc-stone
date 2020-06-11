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

#if !defined(ORTHANC_ENABLE_WASM)
#  error Macro ORTHANC_ENABLE_WASM must be defined
#endif

#if ORTHANC_ENABLE_WASM == 1
#  include "Viewport/WebGLViewportsRegistry.h"
#endif

#include <Logging.h>

namespace OrthancStone
{
  void StoneInitialize(void* pluginContext);

  inline void StoneInitialize()
  {
    StoneInitialize(NULL);
  }

  void StoneFinalize();

#if ORTHANC_ENABLE_WASM == 1
  void SetWebGLViewportsRegistryTimeout(double timeout);
#endif

#if ORTHANC_ENABLE_WASM == 1
  WebGLViewportsRegistry& GetWebGLViewportsRegistry();
#endif
}
