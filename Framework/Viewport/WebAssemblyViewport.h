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
#include "../Scene2D/CairoCompositor.h"
#include "ViewportBase.h"

namespace OrthancStone
{
  class WebAssemblyViewport : public ViewportBase
  {
  public:
    WebAssemblyViewport(const std::string& identifier)
      : ViewportBase(identifier)
    {
    }

    WebAssemblyViewport(const std::string& identifier,
                        boost::shared_ptr<Scene2D>& scene)
      : ViewportBase(identifier, scene)
    {
    }
  };

  class WebAssemblyOpenGLViewport : public WebAssemblyViewport
  {
  private:
    OpenGL::WebAssemblyOpenGLContext  context_;
    std::auto_ptr<OpenGLCompositor>   compositor_;

  public:
    WebAssemblyOpenGLViewport(const std::string& canvas);
    
    WebAssemblyOpenGLViewport(const std::string& canvas,
                              boost::shared_ptr<Scene2D>& scene);
    
    // This function must be called each time the browser window is resized
    void UpdateSize();

    virtual ICompositor& GetCompositor()
    {
      return *compositor_;
    }

    bool OpenGLContextLost();
    bool OpenGLContextRestored();

  private:
    void RegisterContextCallbacks();
  };

  class WebAssemblyCairoViewport : public WebAssemblyViewport
  {
  private:
    CairoCompositor                  compositor_;
    std::string                      canvas_;

  public:
    WebAssemblyCairoViewport(const std::string& canvas);
    
    WebAssemblyCairoViewport(const std::string& canvas,
                             boost::shared_ptr<Scene2D>& scene);
    
    void UpdateSize(); 

    virtual void Refresh();

    virtual ICompositor& GetCompositor()
    {
      return compositor_;
    }
  };

}
