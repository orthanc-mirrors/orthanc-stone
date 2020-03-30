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

#if !defined(ORTHANC_ENABLE_WASM)
#  error Macro ORTHANC_ENABLE_WASM must be defined
#endif

#if ORTHANC_ENABLE_WASM != 1
#  error This file can only be used if targeting WebAssembly
#endif

#include "IViewport.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace OrthancStone
{
  class WebAssemblyViewport : public IViewport,
                              public boost::enable_shared_from_this<WebAssemblyViewport>

  {
  private:
    class WasmLock;
    
    std::string                           shortCanvasId_;
    std::string                           fullCanvasId_;
    std::unique_ptr<ICompositor>          compositor_;
    std::unique_ptr<ViewportController>   controller_;
    std::unique_ptr<IViewportInteractor>  interactor_;
    bool                                  enableEmscriptenEvents_;

    static EM_BOOL OnRequestAnimationFrame(double time, void *userData);
    
    static EM_BOOL OnResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData);

    static EM_BOOL OnMouseDown(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
    
    static EM_BOOL OnMouseMove(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
    
    static EM_BOOL OnMouseUp(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);

  protected:
    void Invalidate();
    
    void ClearCompositor()
    {
      compositor_.reset();
    }

    bool HasCompositor() const
    {
      return compositor_.get() != NULL;
    }

    void AcquireCompositor(ICompositor* compositor /* takes ownership */);

    virtual void Paint(ICompositor& compositor,
                       ViewportController& controller) = 0;

    virtual void UpdateSize(ICompositor& compositor) = 0;

    /**
    The second argument is temporary and should be deleted once the migration 
    to interactors is finished.
    */
    WebAssemblyViewport(const std::string& canvasId, 
                        bool enableEmscriptenEvents = true);

    void PostConstructor();

  public:
    virtual ILock* Lock() ORTHANC_OVERRIDE;

    ~WebAssemblyViewport();


    /**
    This method takes ownership
    */
    void AcquireInteractor(IViewportInteractor* interactor);

    const std::string& GetShortCanvasId() const
    {
      return shortCanvasId_;
    }

    const std::string& GetFullCanvasId() const
    {
      return fullCanvasId_;
    }
  };
}
