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

#if !defined(ORTHANC_ENABLE_SDL)
#  error Macro ORTHANC_ENABLE_SDL must be defined
#endif

#if ORTHANC_ENABLE_SDL != 1
#  error SDL must be enabled to use this file
#endif

#if !defined(ORTHANC_ENABLE_OPENGL)
#  error The macro ORTHANC_ENABLE_OPENGL must be defined
#endif

#if ORTHANC_ENABLE_OPENGL != 1
#  error Support for OpenGL is disabled
#endif

#include "../../Applications/Sdl/SdlOpenGLContext.h"
#include "../Scene2D/OpenGLCompositor.h"
#include "ViewportBase.h"

namespace OrthancStone
{
  class SdlViewport : public ViewportBase
  {
  private:
    SdlOpenGLContext  context_;
    OpenGLCompositor  compositor_;

  public:
    SdlViewport(const char* title,
                unsigned int width,
                unsigned int height,
                bool allowDpiScaling = true);

    SdlViewport(const char* title,
                unsigned int width,
                unsigned int height,
                boost::shared_ptr<Scene2D>& scene,
                bool allowDpiScaling = true);

    virtual void Refresh()
    {
      compositor_.Refresh();
    }

    virtual unsigned int GetCanvasWidth() const
    {
      return compositor_.GetCanvasWidth();
    }

    virtual unsigned int GetCanvasHeight() const
    {
      return compositor_.GetCanvasHeight();
    }

    OpenGLCompositor& GetCompositor()
    {
      return compositor_;
    }

    SdlOpenGLContext& GetContext()
    {
      return context_;
    }
  };
}