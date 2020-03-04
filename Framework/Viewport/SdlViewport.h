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

// TODO: required for UndoStack injection
// I don't like it either :)
#include <boost/weak_ptr.hpp>

#include <boost/thread/recursive_mutex.hpp>

namespace OrthancStone
{
  class UndoStack;

  class SdlViewport : public IViewport
  {
  private:
    boost::recursive_mutex                 mutex_;
    uint32_t                               refreshEvent_;
    boost::shared_ptr<ViewportController>  controller_;
    std::unique_ptr<ICompositor>             compositor_;

    void SendRefreshEvent();

  protected:
    class SdlLock : public ILock
    {
    private:
      SdlViewport&                        that_;
      boost::recursive_mutex::scoped_lock lock_;

    public:
      SdlLock(SdlViewport& that) :
      that_(that),
      lock_(that.mutex_)
      {
      }

      virtual bool HasCompositor() const ORTHANC_OVERRIDE
      {
        return true;
      }

      virtual ICompositor& GetCompositor() ORTHANC_OVERRIDE;
      
      virtual ViewportController& GetController() ORTHANC_OVERRIDE
      {
        return *that_.controller_;
      }

      virtual void Invalidate() ORTHANC_OVERRIDE
      {
        that_.SendRefreshEvent();
      }
    };

    void ClearCompositor()
    {
      compositor_.reset();
    }

    void AcquireCompositor(ICompositor* compositor /* takes ownership */);

  public:
    SdlViewport();
    SdlViewport(boost::weak_ptr<UndoStack> undoStackW);

    bool IsRefreshEvent(const SDL_Event& event) const
    {
      return (event.type == refreshEvent_);
    }

    virtual ILock* Lock() ORTHANC_OVERRIDE
    {
      return new SdlLock(*this);
    }

    virtual void UpdateSize(unsigned int width,
                            unsigned int height) = 0;

    virtual void ToggleMaximize() = 0;

    // Must be invoked from the main SDL thread
    virtual void Paint() = 0;
  };


  class SdlOpenGLViewport : public SdlViewport
  {
  private:
    SdlOpenGLContext  context_;

  public:
    SdlOpenGLViewport(const char* title,
                      unsigned int width,
                      unsigned int height,
                      bool allowDpiScaling = true);

    SdlOpenGLViewport(const char* title,
                      boost::weak_ptr<UndoStack> undoStackW,
                      unsigned int width,
                      unsigned int height,
                      bool allowDpiScaling = true);

    virtual ~SdlOpenGLViewport();

    virtual void Paint() ORTHANC_OVERRIDE;

    virtual void UpdateSize(unsigned int width, 
                            unsigned int height) ORTHANC_OVERRIDE;

    virtual void ToggleMaximize() ORTHANC_OVERRIDE;
  };


  class SdlCairoViewport : public SdlViewport
  {
  private:
    SdlWindow     window_;
    SDL_Surface*  sdlSurface_;

    void CreateSdlSurfaceFromCompositor(CairoCompositor& compositor);

  public:
    SdlCairoViewport(const char* title,
                     unsigned int width,
                     unsigned int height,
                     bool allowDpiScaling = true);

    virtual ~SdlCairoViewport();

    virtual void Paint() ORTHANC_OVERRIDE;

    virtual void UpdateSize(unsigned int width,
                            unsigned int height) ORTHANC_OVERRIDE;

    virtual void ToggleMaximize() ORTHANC_OVERRIDE;
  };
}
