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
#include "../Scene2D/CairoCompositor.h"
#include "ViewportBase.h"

namespace OrthancStone
{
  class SdlViewport : public ViewportBase
  {
  public:
    SdlViewport(const std::string& identifier)
      : ViewportBase(identifier)
    {}

    SdlViewport(const std::string& identifier,
                boost::shared_ptr<Scene2D>& scene)
      : ViewportBase(identifier, scene)
    {

    }

    virtual SdlWindow& GetWindow() = 0;
  };

  class SdlOpenGLViewport : public SdlViewport
  {
  private:
    SdlOpenGLContext  context_;
    OpenGLCompositor  compositor_;

  public:
    SdlOpenGLViewport(const char* title,
                      unsigned int width,
                      unsigned int height,
                      bool allowDpiScaling = true);

    SdlOpenGLViewport(const char* title,
                      unsigned int width,
                      unsigned int height,
                      boost::shared_ptr<Scene2D>& scene,
                      bool allowDpiScaling = true);


    virtual ICompositor& GetCompositor()
    {
      return compositor_;
    }

    virtual SdlWindow& GetWindow()
    {
      return context_.GetWindow();
    }
  };


  class SdlCairoViewport : public SdlViewport
  {
  private:
    SdlWindow         window_;
    CairoCompositor   compositor_;
    SDL_Surface*      sdlSurface_;

  public:
    SdlCairoViewport(const char* title,
                     unsigned int width,
                     unsigned int height,
                     bool allowDpiScaling = true);

    SdlCairoViewport(const char* title,
                     unsigned int width,
                     unsigned int height,
                     boost::shared_ptr<Scene2D>& scene,
                     bool allowDpiScaling = true);

    ~SdlCairoViewport();

    virtual ICompositor& GetCompositor()
    {
      return compositor_;
    }

    virtual SdlWindow& GetWindow()
    {
      return window_;
    }

    virtual void Refresh();

  };
}
