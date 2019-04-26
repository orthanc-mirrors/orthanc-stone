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


#include "SdlOpenGLWindow.h"

#if ORTHANC_ENABLE_SDL == 1

#include <Core/OrthancException.h>

namespace OrthancStone
{
  SdlOpenGLWindow::SdlOpenGLWindow(const char* title,
                                   unsigned int width,
                                   unsigned int height) :
    window_(title, width, height, true /* enable OpenGL */)
  {
    context_ = SDL_GL_CreateContext(window_.GetObject());
    
    if (context_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                      "Cannot initialize OpenGL");
    }
  }

  
  SdlOpenGLWindow::~SdlOpenGLWindow()
  {
    SDL_GL_DeleteContext(context_);
  }


  void SdlOpenGLWindow::MakeCurrent()
  {
    if (SDL_GL_MakeCurrent(window_.GetObject(), context_) != 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                      "Cannot set current OpenGL context");
    }

    // This makes our buffer swap syncronized with the monitor's vertical refresh
    SDL_GL_SetSwapInterval(1);
  }

  
  void SdlOpenGLWindow::SwapBuffer()
  {
    // Swap our buffer to display the current contents of buffer on screen
    SDL_GL_SwapWindow(window_.GetObject());
  }

  
  unsigned int SdlOpenGLWindow::GetCanvasWidth()
  {
    int w = 0;
    SDL_GL_GetDrawableSize(window_.GetObject(), &w, NULL);
    return static_cast<unsigned int>(w);
  }

  
  unsigned int SdlOpenGLWindow::GetCanvasHeight()
  {
    int h = 0;
    SDL_GL_GetDrawableSize(window_.GetObject(), NULL, &h);
    return static_cast<unsigned int>(h);
  }
}

#endif
