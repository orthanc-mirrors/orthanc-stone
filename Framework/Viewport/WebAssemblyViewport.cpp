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


#include "WebAssemblyViewport.h"

#include "../StoneException.h"

#include <emscripten/html5.h>

namespace OrthancStone
{
  WebAssemblyOpenGLViewport::WebAssemblyOpenGLViewport(const std::string& canvas) 
    : WebAssemblyViewport(canvas)
    , context_(canvas)
    , cssWidth_(0)    // will be set in Refresh()
    , cssHeight_(0)   // ditto
    , pixelWidth_(0)  // ditto
    , pixelHeight_(0) // ditto
  {
    compositor_.reset(new OpenGLCompositor(context_, GetScene()));
    RegisterContextCallbacks();
  }

  WebAssemblyOpenGLViewport::WebAssemblyOpenGLViewport(const std::string& canvas,
    boost::shared_ptr<Scene2D>& scene) 
    : WebAssemblyViewport(canvas, scene)
    , context_(canvas)
    , cssWidth_(0)    // will be set in Refresh()
    , cssHeight_(0)   // ditto
    , pixelWidth_(0)  // ditto
    , pixelHeight_(0) // ditto
  {
    compositor_.reset(new OpenGLCompositor(context_, GetScene()));
    RegisterContextCallbacks();
  }

  void WebAssemblyOpenGLViewport::UpdateSize()
  {
    context_.UpdateSize();  // First read the size of the canvas

    if (compositor_.get() != NULL)
    {
      compositor_->Refresh();  // Then refresh the content of the canvas
    }
  }

  /*
  typedef EM_BOOL (*em_webgl_context_callback)(int eventType, const void *reserved, void *userData);

  EMSCRIPTEN_EVENT_WEBGLCONTEXTLOST EMSCRIPTEN_EVENT_WEBGLCONTEXTRESTORED

  EMSCRIPTEN_RESULT emscripten_set_webglcontextlost_callback(
    const char *target, void *userData, EM_BOOL useCapture, em_webgl_context_callback callback)

  EMSCRIPTEN_RESULT emscripten_set_webglcontextrestored_callback(
    const char *target, void *userData, EM_BOOL useCapture, em_webgl_context_callback callback)

  */

  EM_BOOL WebAssemblyOpenGLViewport_OpenGLContextLost_callback(
    int eventType, const void* reserved, void* userData)
  {
    ORTHANC_ASSERT(eventType == EMSCRIPTEN_EVENT_WEBGLCONTEXTLOST);
    WebAssemblyOpenGLViewport* viewport = reinterpret_cast<WebAssemblyOpenGLViewport*>(userData);
    return viewport->OpenGLContextLost();
  }

  EM_BOOL WebAssemblyOpenGLViewport_OpenGLContextRestored_callback(
    int eventType, const void* reserved, void* userData)
  {
    ORTHANC_ASSERT(eventType == EMSCRIPTEN_EVENT_WEBGLCONTEXTRESTORED);
    WebAssemblyOpenGLViewport* viewport = reinterpret_cast<WebAssemblyOpenGLViewport*>(userData);
    return viewport->OpenGLContextRestored();
  }

  void WebAssemblyOpenGLViewport::DisableCompositor()
  {
    compositor_.reset();
  }

  ICompositor& WebAssemblyOpenGLViewport::GetCompositor()
  {
    if (compositor_.get() == NULL)
    {
      // "HasCompositor()" should have been called
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return *compositor_;
    }
  }

  void WebAssemblyOpenGLViewport::UpdateSizeIfNeeded()
  {
    bool needsRefresh = false;
    std::string canvasId = GetCanvasIdentifier();

    {
      double cssWidth = 0;
      double cssHeight = 0;
      EMSCRIPTEN_RESULT res = EMSCRIPTEN_RESULT_SUCCESS;
      res =
        emscripten_get_element_css_size(canvasId.c_str(), &cssWidth, &cssHeight);

      if (res == EMSCRIPTEN_RESULT_SUCCESS)
      {
        if ((cssWidth != cssWidth_) || (cssHeight != cssHeight_))
        {
          cssWidth_ = cssWidth;
          cssHeight_ = cssHeight;
          needsRefresh = true;
        }
      }
    }

    {
      int pixelWidth = 0;
      int pixelHeight = 0;
      EMSCRIPTEN_RESULT res = EMSCRIPTEN_RESULT_SUCCESS;
      res =
        emscripten_get_canvas_element_size(canvasId.c_str(), &pixelWidth, &pixelHeight);

      if (res == EMSCRIPTEN_RESULT_SUCCESS)
      {
        if ((pixelWidth != pixelWidth_) || (pixelHeight != pixelHeight_))
        {
          pixelWidth_ = pixelWidth;
          pixelHeight_ = pixelHeight;
          needsRefresh = true;
        }
      }
    }

    if (needsRefresh)
      UpdateSize();
  }

  void WebAssemblyOpenGLViewport::Refresh()
  {
    try
    {
      // first, we check if the canvas size (both css size in css pixels and
      // backing store) have changed. if so, we call UpdateSize to deal with
      // it

      LOG(TRACE) << "WebAssemblyOpenGLViewport::Refresh";

      // maybe the canvas size has changed and we need to update the 
      // canvas backing store size
      UpdateSizeIfNeeded();
           
      if (HasCompositor())
      {
        GetCompositor().Refresh();
      }
      else
      {
        // this block was added because of (perceived?) bugs in the 
        // browser where the WebGL contexts are NOT automatically restored 
        // after being lost. 
        // the WebGL context has been lost. Sce 

        //LOG(ERROR) << "About to call WebAssemblyOpenGLContext::TryRecreate().";
        //LOG(ERROR) << "Before calling it, isContextLost == " << context_.IsContextLost();

        if (!context_.IsContextLost())
        {
          LOG(TRACE) << "Context restored!";
          //LOG(ERROR) << "After calling it, isContextLost == " << context_.IsContextLost();
          RestoreCompositor();
          UpdateSize();
        }
      }
    }
    catch (const StoneException& e)
    {
      if (e.GetErrorCode() == ErrorCode_WebGLContextLost)
      {
        LOG(WARNING) << "Context is lost! Compositor will be disabled.";
        DisableCompositor();
        // we now need to wait for the "context restored" callback
      }
      else
      {
        throw;
      }
    }
    catch (...)
    {
      // something else nasty happened
      throw;
    }
  }

  void WebAssemblyOpenGLViewport::RestoreCompositor()
  {
    // the context must have been restored!
    ORTHANC_ASSERT(!context_.IsContextLost());
    if (compositor_.get() == NULL)
    {
      compositor_.reset(new OpenGLCompositor(context_, GetScene()));
    }
    else
    {
      LOG(WARNING) << "RestoreCompositor() called for \"" << GetCanvasIdentifier() << "\" while it was NOT lost! Nothing done.";
    }
  }

  bool WebAssemblyOpenGLViewport::OpenGLContextLost()
  {
    LOG(ERROR) << "WebAssemblyOpenGLViewport::OpenGLContextLost() for canvas: " << GetCanvasIdentifier();
    DisableCompositor();
    return true;
  }

  bool WebAssemblyOpenGLViewport::OpenGLContextRestored()
  {
    LOG(ERROR) << "WebAssemblyOpenGLViewport::OpenGLContextRestored() for canvas: " << GetCanvasIdentifier();
    
    // maybe the context has already been restored by other means (the 
    // Refresh() function)
    if (!HasCompositor())
    {
      RestoreCompositor();
      UpdateSize();
    }
    return false;
  }

  void WebAssemblyOpenGLViewport::RegisterContextCallbacks()
  {
#if 0
    // DISABLED ON 2019-08-20 and replaced by external JS calls because I could
    // not get emscripten API to work
    // TODO: what's the impact of userCapture=true ?
    const char* canvasId = GetCanvasIdentifier().c_str();
    void* that = reinterpret_cast<void*>(this);
    EMSCRIPTEN_RESULT status = EMSCRIPTEN_RESULT_SUCCESS;

    //status = emscripten_set_webglcontextlost_callback(canvasId, that, true, WebAssemblyOpenGLViewport_OpenGLContextLost_callback);
    //if (status != EMSCRIPTEN_RESULT_SUCCESS)
    //{
    //  std::stringstream ss;
    //  ss << "Error while calling emscripten_set_webglcontextlost_callback for: \"" << GetCanvasIdentifier() << "\"";
    //  std::string msg = ss.str();
    //  LOG(ERROR) << msg;
    //  ORTHANC_ASSERT(false, msg.c_str());
    //}

    status = emscripten_set_webglcontextrestored_callback(canvasId, that, true, WebAssemblyOpenGLViewport_OpenGLContextRestored_callback);
    if (status != EMSCRIPTEN_RESULT_SUCCESS)
    {
      std::stringstream ss;
      ss << "Error while calling emscripten_set_webglcontextrestored_callback for: \"" << GetCanvasIdentifier() << "\"";
      std::string msg = ss.str();
      LOG(ERROR) << msg;
      ORTHANC_ASSERT(false, msg.c_str());
    }
    LOG(TRACE) << "WebAssemblyOpenGLViewport::RegisterContextCallbacks() SUCCESS!!!";
#endif
  }

  WebAssemblyCairoViewport::WebAssemblyCairoViewport(const std::string& canvas) :
    WebAssemblyViewport(canvas),
    canvas_(canvas),
    compositor_(GetScene(), 1024, 768)
  {
  }

  WebAssemblyCairoViewport::WebAssemblyCairoViewport(const std::string& canvas,
    boost::shared_ptr<Scene2D>& scene) :
    WebAssemblyViewport(canvas, scene),
    canvas_(canvas),
    compositor_(GetScene(), 1024, 768)
  {
  }

  void WebAssemblyCairoViewport::UpdateSize()
  {
    LOG(INFO) << "updating cairo viewport size";
    double w, h;
    emscripten_get_element_css_size(canvas_.c_str(), &w, &h);

    /**
     * Emscripten has the function emscripten_get_element_css_size()
     * to query the width and height of a named HTML element. I'm
     * calling this first to get the initial size of the canvas DOM
     * element, and then call emscripten_set_canvas_size() to
     * initialize the framebuffer size of the canvas to the same
     * size as its DOM element.
     * https://floooh.github.io/2017/02/22/emsc-html.html
     **/
    unsigned int canvasWidth = 0;
    unsigned int canvasHeight = 0;

    if (w > 0 ||
      h > 0)
    {
      canvasWidth = static_cast<unsigned int>(boost::math::iround(w));
      canvasHeight = static_cast<unsigned int>(boost::math::iround(h));
    }

    emscripten_set_canvas_element_size(canvas_.c_str(), canvasWidth, canvasHeight);
    compositor_.UpdateSize(canvasWidth, canvasHeight);
  }

  void WebAssemblyCairoViewport::Refresh()
  {
    LOG(INFO) << "refreshing cairo viewport, TODO: blit to the canvans.getContext('2d')";
    GetCompositor().Refresh();
  }
}
