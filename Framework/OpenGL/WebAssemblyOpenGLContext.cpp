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


#include "WebAssemblyOpenGLContext.h"

#include <Core/OrthancException.h>

#include <emscripten/html5.h>
#include <emscripten/em_asm.h>

#include <boost/math/special_functions/round.hpp>

namespace OrthancStone
{
  namespace OpenGL
  {
    class WebAssemblyOpenGLContext::PImpl
    {
    private:
      std::string                      canvas_;
      EMSCRIPTEN_WEBGL_CONTEXT_HANDLE  context_;
      unsigned int                     canvasWidth_;
      unsigned int                     canvasHeight_;

    public:
      PImpl(const std::string& canvas) :
        canvas_(canvas)
      {
        // Context configuration
        EmscriptenWebGLContextAttributes attr; 
        emscripten_webgl_init_context_attributes(&attr);

        context_ = emscripten_webgl_create_context(canvas.c_str(), &attr);
        if (context_ < 0)
        {
          std::string message("Cannot create an OpenGL context for canvas: ");
          message += canvas;
          LOG(ERROR) << message;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError, message);
        }

        UpdateSize();
      }

      ~PImpl()
      {
        emscripten_webgl_destroy_context(context_);
      }

      const std::string& GetCanvasIdentifier() const
      {
        return canvas_;
      }

      void MakeCurrent()
      {
        if (emscripten_is_webgl_context_lost(context_))
        {
          LOG(ERROR) << "OpenGL context has been lost! for canvas: " << canvas_;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
            "OpenGL context has been lost!");
        }

        if (emscripten_webgl_make_context_current(context_) != EMSCRIPTEN_RESULT_SUCCESS)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Cannot set the OpenGL context");
        }
      }

      void SwapBuffer() 
      {
        /**
         * "Rendered WebGL content is implicitly presented (displayed to
         * the user) on the canvas when the event handler that renders with
         * WebGL returns back to the browser event loop."
         * https://emscripten.org/docs/api_reference/html5.h.html#webgl-context
         *
         * Could call "emscripten_webgl_commit_frame()" if
         * "explicitSwapControl" option were set to "true".
         **/
      }

      unsigned int GetCanvasWidth() const
      {
        return canvasWidth_;
      }

      unsigned int GetCanvasHeight() const
      {
        return canvasHeight_;
      }

      void UpdateSize()
      {
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

        if (w <= 0 ||
            h <= 0)
        {
          canvasWidth_ = 0;
          canvasHeight_ = 0;
        }
        else
        {
          canvasWidth_ = static_cast<unsigned int>(boost::math::iround(w));
          canvasHeight_ = static_cast<unsigned int>(boost::math::iround(h));
        }
    
        emscripten_set_canvas_element_size(canvas_.c_str(), canvasWidth_, canvasHeight_);
      }
    };


    WebAssemblyOpenGLContext::WebAssemblyOpenGLContext(const std::string& canvas) :
      pimpl_(new PImpl(canvas))
    {
    }

    void WebAssemblyOpenGLContext::MakeCurrent()
    {
      assert(pimpl_.get() != NULL);
      pimpl_->MakeCurrent();
    }


    void WebAssemblyOpenGLContext::SwapBuffer() 
    {
      assert(pimpl_.get() != NULL);
      pimpl_->SwapBuffer();
    }

    unsigned int WebAssemblyOpenGLContext::GetCanvasWidth() const
    {
      assert(pimpl_.get() != NULL);
      return pimpl_->GetCanvasWidth();
    }

    unsigned int WebAssemblyOpenGLContext::GetCanvasHeight() const
    {
      assert(pimpl_.get() != NULL);
      return pimpl_->GetCanvasHeight();
    }

    void WebAssemblyOpenGLContext::UpdateSize()
    {
      assert(pimpl_.get() != NULL);
      pimpl_->UpdateSize();
    }

    const std::string& WebAssemblyOpenGLContext::GetCanvasIdentifier() const
    {
      assert(pimpl_.get() != NULL);
      return pimpl_->GetCanvasIdentifier();
    }
  }
}
