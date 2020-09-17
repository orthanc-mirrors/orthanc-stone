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


#if defined(ORTHANC_BUILDING_STONE_LIBRARY) && ORTHANC_BUILDING_STONE_LIBRARY == 1
#  include "WebAssemblyViewport.h"
#  include "DefaultViewportInteractor.h"
#  include "../Toolbox/GenericToolbox.h"
#  include "../Scene2DViewport/ViewportController.h"
#else
// This is the case when using the WebAssembly side module, and this
// source file must be compiled within the WebAssembly main module
#  include <Viewport/WebAssemblyViewport.h>
#  include <Toolbox/GenericToolbox.h>
#  include <Scene2DViewport/ViewportController.h>
#  include <Viewport/DefaultViewportInteractor.h>
#endif


#include <OrthancException.h>

#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace OrthancStone
{
  static void ConvertMouseEvent(PointerEvent& target,
                                const EmscriptenMouseEvent& source,
                                const ICompositor& compositor)
  {
    int x = static_cast<int>(source.targetX);
    int y = static_cast<int>(source.targetY);

    switch (source.button)
    {
      case 0:
        target.SetMouseButton(MouseButton_Left);
        break;

      case 1:
        target.SetMouseButton(MouseButton_Middle);
        break;

      case 2:
        target.SetMouseButton(MouseButton_Right);
        break;

      default:
        target.SetMouseButton(MouseButton_None);
        break;
    }
      
    target.AddPosition(compositor.GetPixelCenterCoordinates(x, y));
    target.SetAltModifier(source.altKey);
    target.SetControlModifier(source.ctrlKey);
    target.SetShiftModifier(source.shiftKey);
  }


  class WebAssemblyViewport::WasmLock : public ILock
  {
  private:
    WebAssemblyViewport& that_;

  public:
    WasmLock(WebAssemblyViewport& that) :
      that_(that)
    {
    }

    virtual bool HasCompositor() const ORTHANC_OVERRIDE
    {
      return that_.compositor_.get() != NULL;
    }

    virtual ICompositor& GetCompositor() ORTHANC_OVERRIDE
    {
      if (that_.compositor_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return *that_.compositor_;
      }
    }

    virtual ViewportController& GetController() ORTHANC_OVERRIDE
    {
      assert(that_.controller_);
      return *that_.controller_;
    }

    virtual void Invalidate() ORTHANC_OVERRIDE
    {
      that_.Invalidate();
    }
  };


  EM_BOOL WebAssemblyViewport::OnRequestAnimationFrame(double time, void *userData)
  {
    LOG(TRACE) << __func__;
    WebAssemblyViewport* that = reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that->compositor_.get() != NULL &&
        that->controller_ /* should always be true */)
    {
      that->Paint(*that->compositor_, *that->controller_);
    }
      
    LOG(TRACE) << "Exiting: " << __func__;
    return true;
  }

  EM_BOOL WebAssemblyViewport::OnResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
  {
    LOG(TRACE) << __func__;
    WebAssemblyViewport* that = reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that->compositor_.get() != NULL)
    {
      that->UpdateSize(*that->compositor_);
      that->Invalidate();
    }
      
    LOG(TRACE) << "Exiting: " << __func__;
    return true;
  }


  EM_BOOL WebAssemblyViewport::OnMouseDown(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
  {
    WebAssemblyViewport* that = reinterpret_cast<WebAssemblyViewport*>(userData);

    LOG(TRACE) << "mouse down: " << that->GetCanvasCssSelector();      

    if (that->compositor_.get() != NULL &&
        that->interactor_.get() != NULL)
    {
      PointerEvent pointer;
      ConvertMouseEvent(pointer, *mouseEvent, *that->compositor_);

      that->controller_->HandleMousePress(*that->interactor_, pointer,
                                         that->compositor_->GetCanvasWidth(),
                                         that->compositor_->GetCanvasHeight());        
      that->Invalidate();
    }

    LOG(TRACE) << "Exiting: " << __func__;
    return true;
  }

    
  EM_BOOL WebAssemblyViewport::OnMouseMove(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
  {
    WebAssemblyViewport* that = reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that->compositor_.get() != NULL &&
        that->controller_->HasActiveTracker())
    {
      PointerEvent pointer;
      ConvertMouseEvent(pointer, *mouseEvent, *that->compositor_);
      if (that->controller_->HandleMouseMove(pointer))
      {
        that->Invalidate();
      }
    }

    LOG(TRACE) << "Exiting: " << __func__;
    return true;
  }
    
  EM_BOOL WebAssemblyViewport::OnMouseUp(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
  {
    LOG(TRACE) << __func__;
    WebAssemblyViewport* that = reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that->compositor_.get() != NULL)
    {
      PointerEvent pointer;
      ConvertMouseEvent(pointer, *mouseEvent, *that->compositor_);
      that->controller_->HandleMouseRelease(pointer);
      that->Invalidate();
    }

    LOG(TRACE) << "Exiting: " << __func__;
    return true;
  }

  void WebAssemblyViewport::Invalidate()
  {
    if (compositor_.get() != NULL &&
        controller_ /* should always be true */)
    {
      printf("TOTO3\n");
      UpdateSize(*compositor_);
      OnRequestAnimationFrame(0, reinterpret_cast<void*>(this));
    }
    
    //emscripten_request_animation_frame(OnRequestAnimationFrame, reinterpret_cast<void*>(this));
  }

  void WebAssemblyViewport::AcquireCompositor(ICompositor* compositor /* takes ownership */)
  {
    if (compositor == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      compositor_.reset(compositor);
    }
  }

#if DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR == 1
// everything OK..... we're using the new setting
#else
#pragma message("WARNING: DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR is not defined or equal to 0. Stone will use the OLD Emscripten rules for DOM element selection.")
#endif

  WebAssemblyViewport::WebAssemblyViewport(
    const std::string& canvasId, bool enableEmscriptenMouseEvents) :
    canvasId_(canvasId),
#if DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR == 1
    canvasCssSelector_("#" + canvasId),
#else
    canvasCssSelector_(canvasId),
#endif
    interactor_(new DefaultViewportInteractor),
    enableEmscriptenMouseEvents_(enableEmscriptenMouseEvents)
  {
  }

  void WebAssemblyViewport::PostConstructor()
  {
    boost::shared_ptr<IViewport> viewport = shared_from_this();
    controller_.reset(new ViewportController(viewport));

    LOG(INFO) << "Initializing Stone viewport on HTML canvas: " 
              << canvasId_;

    if (canvasId_.empty() ||
        canvasId_[0] == '#')
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
        "The canvas identifier must not start with '#'");
    }

    // Disable right-click on the canvas (i.e. context menu)
    EM_ASM({
        document.getElementById(UTF8ToString($0)).oncontextmenu = 
        function(event)
        {
          event.preventDefault();
        }
      },
      canvasId_.c_str()   // $0
      );

    // It is not possible to monitor the resizing of individual
    // canvas, so we track the full window of the browser
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,
                                   reinterpret_cast<void*>(this),
                                   false,
                                   OnResize);

    if (enableEmscriptenMouseEvents_)
    {

      // if any of this function causes an error in the console, please
      // make sure you are using the new (as of 1.39.x) version of 
      // emscripten element lookup rules( pass 
      // "-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1" to the linker.

      emscripten_set_mousedown_callback(canvasCssSelector_.c_str(),
                                        reinterpret_cast<void*>(this),
                                        false,
                                        OnMouseDown);

      emscripten_set_mousemove_callback(canvasCssSelector_.c_str(),
                                        reinterpret_cast<void*>(this),
                                        false,
                                        OnMouseMove);

      emscripten_set_mouseup_callback(canvasCssSelector_.c_str(),
                                      reinterpret_cast<void*>(this),
                                      false,
                                      OnMouseUp);
    }
  }

  void WebAssemblyViewport::UpdateCanvasSize()
  {
    UpdateSize(*compositor_);
  }

  WebAssemblyViewport::~WebAssemblyViewport()
  {
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW,
                                   reinterpret_cast<void*>(this),
                                   false,
                                   NULL);

    if (enableEmscriptenMouseEvents_)
    {

      emscripten_set_mousedown_callback(canvasCssSelector_.c_str(),
                                        reinterpret_cast<void*>(this),
                                        false,
                                        NULL);

      emscripten_set_mousemove_callback(canvasCssSelector_.c_str(),
                                        reinterpret_cast<void*>(this),
                                        false,
                                        NULL);

      emscripten_set_mouseup_callback(canvasCssSelector_.c_str(),
                                      reinterpret_cast<void*>(this),
                                      false,
                                      NULL);
    }
  }
  
  IViewport::ILock* WebAssemblyViewport::Lock()
  {
    return new WasmLock(*this);
  }
  
  void WebAssemblyViewport::AcquireInteractor(IViewportInteractor* interactor)
  {
    if (interactor == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      interactor_.reset(interactor);
    }
  }
}
