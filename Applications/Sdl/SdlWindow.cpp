/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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


#include "SdlWindow.h"

#if ORTHANC_ENABLE_SDL == 1

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  SdlWindow::SdlWindow(const char* title,
                       unsigned int width,
                       unsigned int height,
                       bool enableOpenGl) :
    maximized_(false)
  {
    // TODO Understand why, with SDL_WINDOW_OPENGL + MinGW32 + Release
    // build mode, the application crashes whenever the SDL window is
    // resized or maximized

    uint32_t windowFlags, rendererFlags;
    if (enableOpenGl)
    {
      windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
      rendererFlags = SDL_RENDERER_ACCELERATED;
    }
    else
    {
      windowFlags = SDL_WINDOW_RESIZABLE;
      rendererFlags = SDL_RENDERER_SOFTWARE;
    }
    
    window_ = SDL_CreateWindow(title,
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               width, height, windowFlags);

    if (window_ == NULL) 
    {
      LOG(ERROR) << "Cannot create the SDL window: " << SDL_GetError();
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    renderer_ = SDL_CreateRenderer(window_, -1, rendererFlags);
    if (!renderer_)
    {
      LOG(ERROR) << "Cannot create the SDL renderer: " << SDL_GetError();
      SDL_DestroyWindow(window_);
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  SdlWindow::~SdlWindow()
  {
    if (renderer_ != NULL)
    { 
      SDL_DestroyRenderer(renderer_);
    }

    if (window_ != NULL)
    { 
      SDL_DestroyWindow(window_);
    }
  }


  unsigned int SdlWindow::GetWidth() const
  {
    int w = -1;
    SDL_GetWindowSize(window_, &w, NULL);

    if (w < 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return static_cast<unsigned int>(w);
    }
  }


  unsigned int SdlWindow::GetHeight() const
  {
    int h = -1;
    SDL_GetWindowSize(window_, NULL, &h);

    if (h < 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
    else
    {
      return static_cast<unsigned int>(h);
    }
  }


  void SdlWindow::Render(SDL_Surface* surface)
  {
    //SDL_RenderClear(renderer_);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture != NULL)
    {
      SDL_RenderCopy(renderer_, texture, NULL, NULL);
      SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer_);
  }


  void SdlWindow::ToggleMaximize()
  {
    if (maximized_)
    {
      SDL_RestoreWindow(window_);
      maximized_ = false;
    }
    else
    {
      SDL_MaximizeWindow(window_);
      maximized_ = true;
    }
  }
}

#endif
