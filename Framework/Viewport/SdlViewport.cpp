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

#include "SdlViewport.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  SdlViewport::SdlViewport()
  {
    refreshEvent_ = SDL_RegisterEvents(1);
    
    if (refreshEvent_ == static_cast<uint32_t>(-1))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  void SdlViewport::SendRefreshEvent()
  {
    SDL_Event event;
    SDL_memset(&event, 0, sizeof(event));
    event.type = refreshEvent_;
    SDL_PushEvent(&event);  // This function is thread-safe, and can be called from other threads safely.
  }

  
  SdlOpenGLViewport::SdlOpenGLViewport(const char* title,
                                       unsigned int width,
                                       unsigned int height,
                                       bool allowDpiScaling) :
    context_(title, width, height, allowDpiScaling)
  {
    compositor_.reset(new OpenGLCompositor(context_, GetScene()));
  }

  void SdlOpenGLViewport::Invalidate()
  {
    SendRefreshEvent();
  }

  void SdlOpenGLViewport::Paint()
  {
    boost::mutex::scoped_lock lock(mutex_);
    compositor_->Refresh();
  }


  SdlCairoViewport::SdlCairoViewport(const char* title,
                                     unsigned int width,
                                     unsigned int height,
                                     bool allowDpiScaling) :
    window_(title, width, height, false /* enable OpenGL */, allowDpiScaling),
    compositor_(GetScene(), width, height),
    sdlSurface_(NULL)
  {
  }

  SdlCairoViewport::~SdlCairoViewport()
  {
    if (sdlSurface_)
    {
      SDL_FreeSurface(sdlSurface_);
    }
  }
  
  void SdlCairoViewport::InvalidateInternal()  // Assumes that the mutex is locked
  {
    compositor_.Refresh();
    CreateSdlSurfaceFromCompositor();
  }

  void SdlCairoViewport::Paint()
  {
    boost::mutex::scoped_lock lock(mutex_);
    
    if (sdlSurface_ != NULL)
    {
      window_.Render(sdlSurface_);
    }    
  }

  void SdlCairoViewport::Invalidate()
  {
    {
      boost::mutex::scoped_lock lock(mutex_);
      InvalidateInternal();
    }

    SendRefreshEvent();
  }

  void SdlCairoViewport::UpdateSize(unsigned int width,
                                    unsigned int height)
  {
    boost::mutex::scoped_lock lock(mutex_);
    compositor_.UpdateSize(width, height);
    InvalidateInternal();
  }
  
  void SdlCairoViewport::CreateSdlSurfaceFromCompositor()  // Assumes that the mutex is locked
  {
    static const uint32_t rmask = 0x00ff0000;
    static const uint32_t gmask = 0x0000ff00;
    static const uint32_t bmask = 0x000000ff;

    const unsigned int width = compositor_.GetCanvas().GetWidth();
    const unsigned int height = compositor_.GetCanvas().GetHeight();
    
    if (sdlSurface_ != NULL)
    {
      if (sdlSurface_->pixels == compositor_.GetCanvas().GetBuffer() &&
          sdlSurface_->w == static_cast<int>(width) &&
          sdlSurface_->h == static_cast<int>(height) &&
          sdlSurface_->pitch == static_cast<int>(compositor_.GetCanvas().GetPitch()))
      {
        // The image from the compositor has not changed, no need to update the surface
        return;
      }
      else
      {
        SDL_FreeSurface(sdlSurface_);
      }
    }

    sdlSurface_ = SDL_CreateRGBSurfaceFrom((void*)(compositor_.GetCanvas().GetBuffer()), width, height, 32,
                                           compositor_.GetCanvas().GetPitch(), rmask, gmask, bmask, 0);
    if (!sdlSurface_)
    {
      LOG(ERROR) << "Cannot create a SDL surface from a Cairo surface";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }
}
