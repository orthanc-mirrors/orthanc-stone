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

#include "../OpenGL/SdlOpenGLContext.h"
#include "../Scene2D/OpenGLCompositor.h"
#include "../Scene2D/CairoCompositor.h"
#include "IViewport.h"

#include <SDL_events.h>

namespace OrthancStone
{
  class SdlViewport : public IViewport
  {
  private:
    uint32_t  refreshEvent_;

  protected:
    void SendRefreshEvent();
    
  public:
    SdlViewport();

    bool IsRefreshEvent(const SDL_Event& event) const
    {
      return (event.type == refreshEvent_);
    }

    virtual void UpdateSize(unsigned int width,
                            unsigned int height) = 0;

    virtual void ToggleMaximize() = 0;
  };


  class SdlOpenGLViewport : public SdlViewport
  {
  private:
    boost::mutex                     mutex_;
    SdlOpenGLContext                 context_;
    std::auto_ptr<OpenGLCompositor>  compositor_;

    class SdlLock : public ILock
    {
    private:
      SdlOpenGLViewport&         that_;
      boost::mutex::scoped_lock  lock_;
      
    public:
      SdlLock(SdlOpenGLViewport& viewport) :
        that_(viewport),
        lock_(viewport.mutex_)
      {
      }

      virtual bool HasCompositor() const ORTHANC_OVERRIDE
      {
        return true;
      }

      virtual ICompositor& GetCompositor() ORTHANC_OVERRIDE
      {
        return *that_.compositor_;
      }
    };
    
  public:
    SdlOpenGLViewport(const char* title,
                      unsigned int width,
                      unsigned int height,
                      bool allowDpiScaling = true);

    virtual void Invalidate() ORTHANC_OVERRIDE;

    virtual void Paint(const Scene2D& scene) ORTHANC_OVERRIDE;

    virtual ILock* Lock() ORTHANC_OVERRIDE
    {
      return new SdlLock(*this);
    }

    virtual void UpdateSize(unsigned int width, unsigned int height) ORTHANC_OVERRIDE
    {
      // nothing to do in OpenGL, the OpenGLCompositor::UpdateSize will be called automatically
    }

    virtual void ToggleMaximize() ORTHANC_OVERRIDE
    {
      boost::mutex::scoped_lock lock(mutex_);
      context_.ToggleMaximize();
    }
  };


  class SdlCairoViewport : public SdlViewport
  {
  private:
    class SdlLock : public ILock
    {
    private:
      SdlCairoViewport&          that_;
      boost::mutex::scoped_lock  lock_;
      
    public:
      SdlLock(SdlCairoViewport& viewport) :
        that_(viewport),
        lock_(viewport.mutex_)
      {
      }

      virtual bool HasCompositor() const ORTHANC_OVERRIDE
      {
        return true;
      }

      virtual ICompositor& GetCompositor() ORTHANC_OVERRIDE
      {
        return that_.compositor_;
      }
    };
 
    boost::mutex      mutex_;
    SdlWindow         window_;
    CairoCompositor   compositor_;
    SDL_Surface*      sdlSurface_;

    void CreateSdlSurfaceFromCompositor();

  public:
    SdlCairoViewport(const char* title,
                     unsigned int width,
                     unsigned int height,
                     bool allowDpiScaling = true);

    ~SdlCairoViewport();

    virtual void Invalidate() ORTHANC_OVERRIDE;

    virtual void Paint(const Scene2D& scene) ORTHANC_OVERRIDE;

    virtual ILock* Lock() ORTHANC_OVERRIDE
    {
      return new SdlLock(*this);
    }

    virtual void UpdateSize(unsigned int width,
                            unsigned int height) ORTHANC_OVERRIDE;

    virtual void ToggleMaximize() ORTHANC_OVERRIDE
    {
      boost::mutex::scoped_lock lock(mutex_);
      window_.ToggleMaximize();
    }
  };
}
