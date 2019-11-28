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


#include "SdlOrthancSurface.h"

#if ORTHANC_ENABLE_SDL == 1

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/Image.h>

#include <SDL_render.h>

namespace OrthancStone
{
  SdlOrthancSurface::SdlOrthancSurface(SdlWindow& window) :
    window_(window),
    sdlSurface_(NULL)
  {
  }


  SdlOrthancSurface::~SdlOrthancSurface()
  {
    if (sdlSurface_)
    {
      SDL_FreeSurface(sdlSurface_);
    }
  }


  void SdlOrthancSurface::SetSize(unsigned int width,
                                  unsigned int height)
  {
    if (image_.get() == NULL ||
        image_->GetWidth() != width ||
        image_->GetHeight() != height)
    {
      image_.reset(new Orthanc::Image(Orthanc::PixelFormat_BGRA32, width, height, true));  // (*)

      if (image_->GetPitch() != image_->GetWidth() * 4)
      {
        // This should have been ensured by setting "forceMinimalPitch" to "true" (*)
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      // TODO Big endian?
      static const uint32_t rmask = 0x00ff0000;
      static const uint32_t gmask = 0x0000ff00;
      static const uint32_t bmask = 0x000000ff;

      if (sdlSurface_)
      {
        SDL_FreeSurface(sdlSurface_);
      }

      sdlSurface_ = SDL_CreateRGBSurfaceFrom(image_->GetBuffer(), width, height, 32,
                                             image_->GetPitch(), rmask, gmask, bmask, 0);
      if (!sdlSurface_)
      {
        LOG(ERROR) << "Cannot create a SDL surface from a Orthanc surface";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }


  Orthanc::ImageAccessor& SdlOrthancSurface::GetImage()
  {
    if (image_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return *image_;
  }


  void SdlOrthancSurface::Render()
  {
    if (image_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    window_.Render(sdlSurface_);
  }
}

#endif
