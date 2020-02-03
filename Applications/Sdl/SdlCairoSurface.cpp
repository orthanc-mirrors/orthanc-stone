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


#include "SdlCairoSurface.h"

#if ORTHANC_ENABLE_SDL == 1

#include <Core/Logging.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  SdlCairoSurface::SdlCairoSurface(SdlWindow& window) :
    window_(window),
    sdlSurface_(NULL)
  {
  }


  SdlCairoSurface::~SdlCairoSurface()
  {
    if (sdlSurface_)
    {
      SDL_FreeSurface(sdlSurface_);
    }
  }


  void SdlCairoSurface::SetSize(unsigned int width,
                                unsigned int height)
  {
    if (cairoSurface_.get() == NULL ||
        cairoSurface_->GetWidth() != width ||
        cairoSurface_->GetHeight() != height)
    {
      cairoSurface_.reset(new CairoSurface(width, height, false /* no alpha */));

      // TODO Big endian?
      static const uint32_t rmask = 0x00ff0000;
      static const uint32_t gmask = 0x0000ff00;
      static const uint32_t bmask = 0x000000ff;

      if (sdlSurface_)
      {
        SDL_FreeSurface(sdlSurface_);
      }

      sdlSurface_ = SDL_CreateRGBSurfaceFrom(cairoSurface_->GetBuffer(), width, height, 32,
                                             cairoSurface_->GetPitch(), rmask, gmask, bmask, 0);
      if (!sdlSurface_)
      {
        LOG(ERROR) << "Cannot create a SDL surface from a Cairo surface";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }


  void SdlCairoSurface::Render(Deprecated::IViewport& viewport)
  {
    if (cairoSurface_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    Orthanc::ImageAccessor target;
    cairoSurface_->GetWriteableAccessor(target);

    if (viewport.Render(target))
    {
      window_.Render(sdlSurface_);
    }
  }
}

#endif
