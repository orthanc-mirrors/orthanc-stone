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

#include <boost/make_shared.hpp>

namespace OrthancStone
{
  SdlOpenGLViewport::SdlOpenGLViewport(const char* title,
                                       unsigned int width,
                                       unsigned int height,
                                       bool allowDpiScaling) :
    SdlViewport(title),
    context_(title, width, height, allowDpiScaling),
    compositor_(context_, GetScene())
  {
  }

  SdlOpenGLViewport::SdlOpenGLViewport(const char* title,
                                       unsigned int width,
                                       unsigned int height,
                                       boost::shared_ptr<Scene2D>& scene,
                                       bool allowDpiScaling) :
    SdlViewport(title, scene),
    context_(title, width, height, allowDpiScaling),
    compositor_(context_, GetScene())
  {
  }


  SdlCairoViewport::SdlCairoViewport(const char* title,
                                     unsigned int width,
                                     unsigned int height,
                                     bool allowDpiScaling) :
    SdlViewport(title),
    window_(title, width, height, false /* enable OpenGL */, allowDpiScaling),
    compositor_(GetScene(), width, height)
  {
    static const uint32_t rmask = 0x00ff0000;
    static const uint32_t gmask = 0x0000ff00;
    static const uint32_t bmask = 0x000000ff;

    sdlSurface_ = SDL_CreateRGBSurfaceFrom((void*)(compositor_.GetCanvas().GetBuffer()), width, height, 32,
                                           compositor_.GetCanvas().GetPitch(), rmask, gmask, bmask, 0);
    if (!sdlSurface_)
    {
      LOG(ERROR) << "Cannot create a SDL surface from a Cairo surface";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  SdlCairoViewport::~SdlCairoViewport()
  {
    if (sdlSurface_)
    {
      SDL_FreeSurface(sdlSurface_);
    }
  }

//  void SdlCairoViewport::SetSize(unsigned int width,
//                                 unsigned int height)
//  {
    //    if (cairoSurface_.get() == NULL ||
    //        cairoSurface_->GetWidth() != width ||
    //        cairoSurface_->GetHeight() != height)
    //    {
    //      cairoSurface_.reset(new CairoSurface(width, height, false /* no alpha */));

    //      // TODO Big endian?
    //      static const uint32_t rmask = 0x00ff0000;
    //      static const uint32_t gmask = 0x0000ff00;
    //      static const uint32_t bmask = 0x000000ff;

    //      if (sdlSurface_)
    //      {
    //        SDL_FreeSurface(sdlSurface_);
    //      }

    //      sdlSurface_ = SDL_CreateRGBSurfaceFrom(cairoSurface_->GetBuffer(), width, height, 32,
    //                                             cairoSurface_->GetPitch(), rmask, gmask, bmask, 0);
    //      if (!sdlSurface_)
    //      {
    //        LOG(ERROR) << "Cannot create a SDL surface from a Cairo surface";
    //        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    //      }
    //    }
//  }


  void SdlCairoViewport::Refresh()
  {
    GetCompositor().Refresh();
    window_.Render(sdlSurface_);
  }


  //  void SdlCairoViewport::Render()
  //  {
  //    Orthanc::ImageAccessor target;
  //    compositor_.GetCanvas() cairoSurface_->GetWriteableAccessor(target);

  //    if (viewport.Render(target))
  //    {
  //      window_.Render(sdlSurface_);
  //    }
  //  }


}
