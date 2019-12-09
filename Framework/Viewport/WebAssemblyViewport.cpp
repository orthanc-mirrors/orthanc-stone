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


#include "WebAssemblyViewport.h"

#include <Core/OrthancException.h>

#include <boost/make_shared.hpp>

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
    WebAssemblyViewport& that = *reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that.compositor_.get() != NULL &&
        that.controller_ /* should always be true */)
    {
      that.Paint(*that.compositor_, *that.controller_);
    }
      
    return true;
  }

    
  EM_BOOL WebAssemblyViewport::OnResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
  {
    WebAssemblyViewport& that = *reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that.compositor_.get() != NULL)
    {
      that.UpdateSize(*that.compositor_);
      that.Invalidate();
    }
      
    return true;
  }


  EM_BOOL WebAssemblyViewport::OnMouseDown(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
  {
    WebAssemblyViewport& that = *reinterpret_cast<WebAssemblyViewport*>(userData);

    LOG(INFO) << "mouse down: " << that.GetFullCanvasId();      

    if (that.compositor_.get() != NULL &&
        that.interactor_.get() != NULL)
    {
      PointerEvent pointer;
      ConvertMouseEvent(pointer, *mouseEvent, *that.compositor_);

      that.controller_->HandleMousePress(*that.interactor_, pointer,
                                         that.compositor_->GetCanvasWidth(),
                                         that.compositor_->GetCanvasHeight());        
      that.Invalidate();
    }

    return true;
  }

    
  EM_BOOL WebAssemblyViewport::OnMouseMove(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
  {
    WebAssemblyViewport& that = *reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that.compositor_.get() != NULL &&
        that.controller_->HasActiveTracker())
    {
      PointerEvent pointer;
      ConvertMouseEvent(pointer, *mouseEvent, *that.compositor_);
      that.controller_->HandleMouseMove(pointer);        
      that.Invalidate();
    }

    return true;
  }

    
  EM_BOOL WebAssemblyViewport::OnMouseUp(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
  {
    WebAssemblyViewport& that = *reinterpret_cast<WebAssemblyViewport*>(userData);

    if (that.compositor_.get() != NULL)
    {
      PointerEvent pointer;
      ConvertMouseEvent(pointer, *mouseEvent, *that.compositor_);
      that.controller_->HandleMouseRelease(pointer);
      that.Invalidate();
    }

    return true;
  }

    
  void WebAssemblyViewport::Invalidate()
  {
    emscripten_request_animation_frame(OnRequestAnimationFrame, this);
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


  WebAssemblyViewport::WebAssemblyViewport(const std::string& canvasId,
                                           const Scene2D* scene) :
    shortCanvasId_(canvasId),
    fullCanvasId_("#" + canvasId),
    interactor_(new DefaultViewportInteractor)
  {
    if (scene == NULL)
    {
      controller_ = boost::make_shared<ViewportController>();
    }
    else
    {
      controller_ = boost::make_shared<ViewportController>(*scene);
    }

    LOG(INFO) << "Initializing Stone viewport on HTML canvas: " << canvasId;

    if (canvasId.empty() ||
        canvasId[0] == '#')
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                      "The canvas identifier must not start with '#'");
    }

    // Disable right-click on the canvas (i.e. context menu)
    EM_ASM({
        document.getElementById(UTF8ToString($0)).oncontextmenu = function(event) {
          event.preventDefault();
        }
      },
      canvasId.c_str()   // $0
      );

    // It is not possible to monitor the resizing of individual
    // canvas, so we track the full window of the browser
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, OnResize);

    emscripten_set_mousedown_callback(fullCanvasId_.c_str(), this, false, OnMouseDown);
    emscripten_set_mousemove_callback(fullCanvasId_.c_str(), this, false, OnMouseMove);
    emscripten_set_mouseup_callback(fullCanvasId_.c_str(), this, false, OnMouseUp);
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
