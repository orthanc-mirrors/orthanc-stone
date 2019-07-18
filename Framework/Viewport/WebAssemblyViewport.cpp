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
#include <emscripten/html5.h>

namespace OrthancStone
{
  WebAssemblyOpenGLViewport::WebAssemblyOpenGLViewport(const std::string& canvas) :
    WebAssemblyViewport(canvas),
    context_(canvas),
    compositor_(context_, GetScene())
  {
  }

    
  WebAssemblyOpenGLViewport::WebAssemblyOpenGLViewport(const std::string& canvas,
                                                       boost::shared_ptr<Scene2D>& scene) :
    WebAssemblyViewport(canvas, scene),
    context_(canvas),
    compositor_(context_, GetScene())
  {
  }
    

  void WebAssemblyOpenGLViewport::UpdateSize()
  {
    context_.UpdateSize();  // First read the size of the canvas
    compositor_.Refresh();  // Then refresh the content of the canvas
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
