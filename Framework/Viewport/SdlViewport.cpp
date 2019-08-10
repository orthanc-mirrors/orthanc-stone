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
    context_(title, width, height, allowDpiScaling)
  {
    compositor_.reset(new OpenGLCompositor(context_, GetScene()));
  }

  SdlOpenGLViewport::SdlOpenGLViewport(const char* title,
                                       unsigned int width,
                                       unsigned int height,
                                       boost::shared_ptr<Scene2D>& scene,
                                       bool allowDpiScaling) :
    SdlViewport(title, scene),
    context_(title, width, height, allowDpiScaling)
  {
    compositor_.reset(new OpenGLCompositor(context_, GetScene()));
  }

  bool SdlOpenGLViewport::OpenGLContextLost()
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }

  void SdlOpenGLViewport::DisableCompositor()
  {
    compositor_.reset(NULL);
  }

  void SdlOpenGLViewport::RestoreCompositor()
  {
    // the context must have been restored!
    ORTHANC_ASSERT(!context_.IsContextLost());

    if (compositor_.get() == NULL)
    {
      compositor_.reset(new OpenGLCompositor(context_, GetScene()));
    }
    else
    {
      std::string windowTitle(SDL_GetWindowTitle(GetWindow().GetObject()));
      LOG(WARNING) << "RestoreCompositor() called for \"" << windowTitle << "\" while it was NOT lost! Nothing done.";
    }
  }

  // extern bool Debug_MustContextBeRestored(std::string title);
  // extern void Debug_Context_ClearRestoreFlag(std::string title);
  // extern void Debug_Context_ClearKillFlag(std::string title);

  bool Debug_SdlOpenGLViewport_Refresh_BP = false;

  void SdlOpenGLViewport::Refresh()
  {
    // <DEBUG CODE USED FOR CONTEXT LOSS RESTORING>
    // try to restore the context if requested
    // Debug_Context_ClearRestoreFlag
    // Debug_SdlOpenGLViewport_Refresh_BP = true;
    // try
    // {
    //   if (Debug_MustContextBeRestored(GetCanvasIdentifier()))
    //   {
    //     // to prevent a bug where the context is both labelled as "to be lost" and "to be restored"
    //     // (occurs when one is hammering away at the keyboard like there's no tomorrow)
    //     Debug_Context_ClearKillFlag(GetCanvasIdentifier());
    //     // this is called manually for loss/restore simulation
    //     context_.RestoreLostContext();
    //     RestoreCompositor();
    //     Debug_Context_ClearRestoreFlag(GetCanvasIdentifier());
    //   }
    // }
    // catch (const OpenGLContextLostException& e)
    // {
    //   LOG(ERROR) << "OpenGLContextLostException in SdlOpenGLViewport::Refresh() part 1";
    // }
    // Debug_SdlOpenGLViewport_Refresh_BP = false;
    // </DEBUG CODE USED FOR CONTEXT LOSS RESTORING>

    try
    {
      // the compositor COULD be dead!
      if (GetCompositor())
        GetCompositor()->Refresh();
    }
    catch (const OpenGLContextLostException& e)
    {
      // we need to wait for the "context restored" callback
      LOG(WARNING) << "Context " << std::hex << e.context_ << " is lost! Compositor will be disabled.";
      DisableCompositor();

      // <DEBUG CODE USED FOR CONTEXT LOSS RESTORING>
      // in case this was externally triggered...
      //Debug_Context_ClearKillFlag(GetCanvasIdentifier());
      // </DEBUG CODE USED FOR CONTEXT LOSS RESTORING>
    }
    catch (...)
    {
      // something else nasty happened
      throw;
    }
  }





  SdlCairoViewport::SdlCairoViewport(const char* title,
                                     unsigned int width,
                                     unsigned int height,
                                     bool allowDpiScaling) :
    SdlViewport(title),
    window_(title, width, height, false /* enable OpenGL */, allowDpiScaling),
    compositor_(GetScene(), width, height)
  {
    UpdateSdlSurfaceSize(width, height);
  }

  void SdlCairoViewport::DisableCompositor()
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }
  
  void SdlCairoViewport::RestoreCompositor()
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }

  SdlCairoViewport::~SdlCairoViewport()
  {
    if (sdlSurface_)
    {
      SDL_FreeSurface(sdlSurface_);
    }
  }
  
  void SdlCairoViewport::Refresh()
  {
    GetCompositor()->Refresh();
    window_.Render(sdlSurface_);
  }

  void SdlCairoViewport::UpdateSize(unsigned int width,
                                    unsigned int height)
  {
    compositor_.UpdateSize(width, height);
    UpdateSdlSurfaceSize(width, height);
    Refresh();
  }
  
  void SdlCairoViewport::UpdateSdlSurfaceSize(unsigned int width,
                                              unsigned int height)
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

}
