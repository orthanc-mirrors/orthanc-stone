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



#include <emscripten.h>
#include <emscripten/html5.h>

// From Stone
#include "../../Applications/Sdl/SdlOpenGLWindow.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/OpenGL/WebAssemblyOpenGLContext.h"

// From Orthanc framework
#include <Core/Images/Image.h>
#include <Core/Logging.h>

#include <stdio.h>

static const unsigned int FONT_SIZE = 32;


void PrepareScene(OrthancStone::Scene2D& scene)
{
  using namespace OrthancStone;

  // Texture of 2x2 size
  if (1)
  {
    Orthanc::Image i(Orthanc::PixelFormat_RGB24, 2, 2, false);
    
    uint8_t *p = reinterpret_cast<uint8_t*>(i.GetRow(0));
    p[0] = 255;
    p[1] = 0;
    p[2] = 0;

    p[3] = 0;
    p[4] = 255;
    p[5] = 0;

    p = reinterpret_cast<uint8_t*>(i.GetRow(1));
    p[0] = 0;
    p[1] = 0;
    p[2] = 255;

    p[3] = 255;
    p[4] = 0;
    p[5] = 0;

    scene.SetLayer(12, new ColorTextureSceneLayer(i));

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-3, 2);
    l->SetPixelSpacing(1.5, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene.SetLayer(14, l.release());
  }

  // Texture of 1x1 size
  if (1)
  {
    Orthanc::Image i(Orthanc::PixelFormat_RGB24, 1, 1, false);
    
    uint8_t *p = reinterpret_cast<uint8_t*>(i.GetRow(0));
    p[0] = 255;
    p[1] = 0;
    p[2] = 0;

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-2, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene.SetLayer(13, l.release());
  }

  // Some lines
  {
    std::auto_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer);

    layer->SetThickness(1);

    PolylineSceneLayer::Chain chain;
    chain.push_back(ScenePoint2D(0 - 0.5, 0 - 0.5));
    chain.push_back(ScenePoint2D(0 - 0.5, 2 - 0.5));
    chain.push_back(ScenePoint2D(2 - 0.5, 2 - 0.5));
    chain.push_back(ScenePoint2D(2 - 0.5, 0 - 0.5));
    layer->AddChain(chain, true);

    chain.clear();
    chain.push_back(ScenePoint2D(-5, -5));
    chain.push_back(ScenePoint2D(5, -5));
    chain.push_back(ScenePoint2D(5, 5));
    chain.push_back(ScenePoint2D(-5, 5));
    layer->AddChain(chain, true);

    double dy = 1.01;
    chain.clear();
    chain.push_back(ScenePoint2D(-4, -4));
    chain.push_back(ScenePoint2D(4, -4 + dy));
    chain.push_back(ScenePoint2D(-4, -4 + 2.0 * dy));
    chain.push_back(ScenePoint2D(4, 2));
    layer->AddChain(chain, false);

    layer->SetColor(0,255, 255);
    scene.SetLayer(50, layer.release());
  }

  // Some text
  if (1)
  {
    std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
    layer->SetText("Hello");
    scene.SetLayer(100, layer.release());
  }
}




namespace OrthancStone
{
  class WebAssemblyCanvas : public boost::noncopyable
  {
  private:
    OpenGL::WebAssemblyOpenGLContext  context_;
    Scene2D                           scene_;
    OpenGLCompositor                  compositor_;

    void SetupEvents(const std::string& canvas);

  public:
    WebAssemblyCanvas(const std::string& canvas) :
      context_(canvas),
      compositor_(context_, scene_)
    {
      compositor_.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, 
                          FONT_SIZE, Orthanc::Encoding_Latin1);
      SetupEvents(canvas);
    }

    Scene2D& GetScene()
    {
      return scene_;
    }

    void UpdateSize()
    {
      context_.UpdateSize();
      compositor_.UpdateSize();
      Refresh();
    }

    void Refresh()
    {
      compositor_.Refresh();
    }

    const std::string& GetCanvasIdentifier() const
    {
      return context_.GetCanvasIdentifier();
    }

    ScenePoint2D GetPixelCenterCoordinates(int x, int y) const
    {
      return compositor_.GetPixelCenterCoordinates(x, y);
    }
  };



  class ActiveTracker : public boost::noncopyable
  {
  private:
    std::auto_ptr<IPointerTracker>  tracker_;
    std::string                     canvasIdentifier_;
    bool                            insideCanvas_;
    
  public:
    ActiveTracker(IPointerTracker* tracker,
                  const WebAssemblyCanvas& canvas) :
      tracker_(tracker),
      canvasIdentifier_(canvas.GetCanvasIdentifier()),
      insideCanvas_(true)
    {
    }
  };
}



static OrthancStone::PointerEvent* ConvertMouseEvent(const EmscriptenMouseEvent& source,
                                                     OrthancStone::WebAssemblyCanvas& canvas)
{
  std::auto_ptr<OrthancStone::PointerEvent> target(new OrthancStone::PointerEvent);

  target->AddPosition(canvas.GetPixelCenterCoordinates(source.clientX, source.clientY));
  target->SetAltModifier(source.altKey);
  target->SetControlModifier(source.ctrlKey);
  target->SetShiftModifier(source.shiftKey);

  return target.release();
}


EM_BOOL OnMouseEvent(int eventType, 
                     const EmscriptenMouseEvent *mouseEvent, 
                     void *userData)
{
  if (userData != NULL)
  {
    OrthancStone::WebAssemblyCanvas& canvas = *reinterpret_cast<OrthancStone::WebAssemblyCanvas*>(userData);

    switch (eventType)
    {
      case EMSCRIPTEN_EVENT_CLICK:
      {
        static unsigned int count = 0;
        char buf[64];
        sprintf(buf, "click %d", count++);

        std::auto_ptr<OrthancStone::TextSceneLayer> layer(new OrthancStone::TextSceneLayer);
        layer->SetText(buf);
        canvas.GetScene().SetLayer(100, layer.release());
        canvas.Refresh();
        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEDOWN:
        LOG(ERROR) << "Mouse down";
        break;

      default:
        break;
    }
  }

  return true;
}


void OrthancStone::WebAssemblyCanvas::SetupEvents(const std::string& canvas)
{
  emscripten_set_click_callback(canvas.c_str(), this, false, OnMouseEvent);
  //emscripten_set_mousedown_callback(canvas.c_str(), this, false, OnMouseEvent);
}




std::auto_ptr<OrthancStone::WebAssemblyCanvas>  canvas1_;
std::auto_ptr<OrthancStone::WebAssemblyCanvas>  canvas2_;
std::auto_ptr<OrthancStone::WebAssemblyCanvas>  canvas3_;
std::auto_ptr<OrthancStone::ActiveTracker>      tracker_;


EM_BOOL OnWindowResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
  if (canvas1_.get() != NULL)
  {
    canvas1_->UpdateSize();
  }
  
  if (canvas2_.get() != NULL)
  {
    canvas2_->UpdateSize();
  }
  
  if (canvas3_.get() != NULL)
  {
    canvas3_->UpdateSize();
  }
  
  return true;
}



extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    OrthancStone::StoneInitialize();
    EM_ASM(window.dispatchEvent(new CustomEvent("WebAssemblyLoaded")););
  }

  EMSCRIPTEN_KEEPALIVE
  void Initialize()
  {
    canvas1_.reset(new OrthancStone::WebAssemblyCanvas("mycanvas1"));
    PrepareScene(canvas1_->GetScene());
    canvas1_->UpdateSize();

    canvas2_.reset(new OrthancStone::WebAssemblyCanvas("mycanvas2"));
    PrepareScene(canvas2_->GetScene());
    canvas2_->UpdateSize();

    canvas3_.reset(new OrthancStone::WebAssemblyCanvas("mycanvas3"));
    PrepareScene(canvas3_->GetScene());
    canvas3_->UpdateSize();

    emscripten_set_resize_callback("#window", NULL, false, OnWindowResize);
  }
}
