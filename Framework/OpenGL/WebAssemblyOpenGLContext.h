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

#if ORTHANC_ENABLE_WASM == 1

#include "IOpenGLContext.h"

#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  namespace OpenGL
  {
    class WebAssemblyOpenGLContext : public OpenGL::IOpenGLContext
    {
    private:
      class PImpl;
      boost::shared_ptr<PImpl>  pimpl_;

    public:
      WebAssemblyOpenGLContext(const std::string& canvas);

      virtual void MakeCurrent();

      virtual void SwapBuffer();

      virtual unsigned int GetCanvasWidth() const;

      virtual unsigned int GetCanvasHeight() const;

      void UpdateSize();
    };
  }
}

#endif
