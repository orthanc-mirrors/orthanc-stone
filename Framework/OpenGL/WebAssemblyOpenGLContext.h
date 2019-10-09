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


#pragma once

#if !defined(ORTHANC_ENABLE_WASM)
#  error Macro ORTHANC_ENABLE_WASM must be defined
#endif

#if ORTHANC_ENABLE_WASM != 1
#  error This file can only be used if targeting WebAssembly
#endif

#if !defined(ORTHANC_ENABLE_OPENGL)
#  error The macro ORTHANC_ENABLE_OPENGL must be defined
#endif

#if ORTHANC_ENABLE_OPENGL != 1
#  error Support for OpenGL is disabled
#endif

#include "IOpenGLContext.h"
#include "IOpenGLContextLossMonitor.h"

#include <Core/Enumerations.h>

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  namespace OpenGL
  {
    class WebAssemblyOpenGLContext :
      public OpenGL::IOpenGLContext,
      public OpenGL::IOpenGLContextLossMonitor
    {
    private:
      class PImpl;
      boost::shared_ptr<PImpl>  pimpl_;

    public:
      WebAssemblyOpenGLContext(const std::string& canvas);

      virtual bool IsContextLost() ORTHANC_OVERRIDE;

      virtual void SetLostContext() ORTHANC_OVERRIDE;
      virtual void RestoreLostContext() ORTHANC_OVERRIDE;

      virtual void MakeCurrent() ORTHANC_OVERRIDE;

      virtual void SwapBuffer() ORTHANC_OVERRIDE;

      virtual unsigned int GetCanvasWidth() const ORTHANC_OVERRIDE;

      virtual unsigned int GetCanvasHeight() const ORTHANC_OVERRIDE;

      virtual void* DebugGetInternalContext() const ORTHANC_OVERRIDE;

      /**
      Returns true if the underlying context has been successfully recreated
      */
      //bool TryRecreate();

      void UpdateSize();

      const std::string& GetCanvasIdentifier() const;
    };
  }
}
