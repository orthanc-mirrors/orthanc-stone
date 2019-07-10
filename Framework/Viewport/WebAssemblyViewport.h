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

#include "../OpenGL/WebAssemblyOpenGLContext.h"
#include "../Scene2D/OpenGLCompositor.h"
#include "ViewportBase.h"

namespace OrthancStone
{
  class WebAssemblyViewport : public ViewportBase
  {
  private:
    OpenGL::WebAssemblyOpenGLContext  context_;
    OpenGLCompositor                  compositor_;

  public:
    WebAssemblyViewport(const std::string& canvas);
    
    WebAssemblyViewport(const std::string& canvas,
                        boost::shared_ptr<Scene2D>& scene);
    
    virtual void Refresh()
    {
      compositor_.Refresh();
    }

    virtual unsigned int GetCanvasWidth() const
    {
      return context_.GetCanvasWidth();
    }

    virtual unsigned int GetCanvasHeight() const
    {
      return context_.GetCanvasHeight();
    }

    // This function must be called each time the browser window is resized
    void UpdateSize();

    OpenGLCompositor& GetCompositor()
    {
      return compositor_;
    }
  };
}
