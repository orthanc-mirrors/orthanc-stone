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
#include "../../Framework/Scene2DViewport/ViewportController.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/OpenGL/WebAssemblyOpenGLContext.h"

// From Orthanc framework
#include <Core/Images/Image.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/make_shared.hpp>

#include <stdio.h>

static const unsigned int FONT_SIZE = 32;

using boost::shared_ptr;
using boost::make_shared;

void PrepareScene(OrthancStone::Scene2DPtr scene)
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

    scene->SetLayer(12, new ColorTextureSceneLayer(i));

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-3, 2);
    l->SetPixelSpacing(1.5, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene->SetLayer(14, l.release());
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
    scene->SetLayer(13, l.release());
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
    scene->SetLayer(50, layer.release());
  }

  // Some text
  if (1)
  {
    std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
    layer->SetText("Hello");
    scene->SetLayer(100, layer.release());
  }
}

namespace OrthancStone
{
  class WebAssemblyViewport : public boost::noncopyable
  {
  private:
    // the construction order is important because compositor_
    // will hold a reference to the scene that belong to the 
    // controller_ object
    OpenGL::WebAssemblyOpenGLContext  context_;
    ViewportControllerPtr             controller_;
    OpenGLCompositor                  compositor_;

    void SetupEvents(const std::string& canvas);

  public:
    WebAssemblyViewport(const std::string& canvas) :
      context_(canvas),
      controller_(make_shared<ViewportController>(broker)),
      compositor_(context_, *controller_->GetScene())
    {
      compositor_.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, 
                          FONT_SIZE, Orthanc::Encoding_Latin1);
      SetupEvents(canvas);
    }

    Scene2DPtr GetScene()
    {
      return controller_->GetScene();
    }

    ViewportControllerPtr GetController()
    {
      return controller_;
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

    unsigned int GetCanvasWidth() const
    {
      return context_.GetCanvasWidth();
    }

    unsigned int GetCanvasHeight() const
    {
      return context_.GetCanvasHeight();
    }
  };

  class ActiveTracker : public boost::noncopyable
  {
  private:
    OrthancStone::FlexiblePointerTrackerPtr tracker_;
    std::string                             canvasIdentifier_;
    bool                                    insideCanvas_;
    
  public:
    ActiveTracker(FlexiblePointerTrackerPtr tracker,
                  const WebAssemblyViewport& viewport) :
      tracker_(tracker),
      canvasIdentifier_(viewport.GetCanvasIdentifier()),
      insideCanvas_(true)
    {
      if (tracker_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    bool IsAlive() const
    {
      return tracker_->IsAlive();
    }

    void PointerMove(const PointerEvent& event)
    {
      tracker_->PointerMove(event);
    }

    void PointerUp(const PointerEvent& event)
    {
      tracker_->PointerUp(event);
    }
  };
}

static OrthancStone::PointerEvent* ConvertMouseEvent(
  const EmscriptenMouseEvent&        source,
  OrthancStone::WebAssemblyViewport& viewport)
{
  std::auto_ptr<OrthancStone::PointerEvent> target(
    new OrthancStone::PointerEvent);

  target->AddPosition(viewport.GetPixelCenterCoordinates(
    source.targetX, source.targetY));
  target->SetAltModifier(source.altKey);
  target->SetControlModifier(source.ctrlKey);
  target->SetShiftModifier(source.shiftKey);

  return target.release();
}

std::auto_ptr<OrthancStone::ActiveTracker> tracker_;

EM_BOOL OnMouseEvent(int eventType, 
                     const EmscriptenMouseEvent *mouseEvent, 
                     void *userData)
{
  if (mouseEvent != NULL &&
      userData != NULL)
  {
    OrthancStone::WebAssemblyViewport& viewport = 
      *reinterpret_cast<OrthancStone::WebAssemblyViewport*>(userData);

    switch (eventType)
    {
      case EMSCRIPTEN_EVENT_CLICK:
      {
        static unsigned int count = 0;
        char buf[64];
        sprintf(buf, "click %d", count++);

        std::auto_ptr<OrthancStone::TextSceneLayer> layer(new OrthancStone::TextSceneLayer);
        layer->SetText(buf);
        viewport.GetScene()->SetLayer(100, layer.release());
        viewport.Refresh();
        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEDOWN:
      {
        OrthancStone::FlexiblePointerTrackerPtr t;

        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));

          switch (mouseEvent->button)
          {
            case 0:  // Left button
              emscripten_console_log("Creating RotateSceneTracker");
              t.reset(new OrthancStone::RotateSceneTracker(
                viewport.GetController(), *event));
              break;

            case 1:  // Middle button
              emscripten_console_log("Creating PanSceneTracker");
              LOG(INFO) << "Creating PanSceneTracker" ;
              t.reset(new OrthancStone::PanSceneTracker(
                viewport.GetController(), *event));
              break;

            case 2:  // Right button
              emscripten_console_log("Creating ZoomSceneTracker");
              t.reset(new OrthancStone::ZoomSceneTracker(
                viewport.GetController(), *event, viewport.GetCanvasWidth()));
              break;

            default:
              break;
          }
        }

        if (t.get() != NULL)
        {
          tracker_.reset(
            new OrthancStone::ActiveTracker(t, viewport));
          viewport.Refresh();
        }

        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEMOVE:
        if (tracker_.get() != NULL)
        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));
          tracker_->PointerMove(*event);
          viewport.Refresh();
        }
        break;

      case EMSCRIPTEN_EVENT_MOUSEUP:
        if (tracker_.get() != NULL)
        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));
          tracker_->PointerUp(*event);
          viewport.Refresh();
          if (!tracker_->IsAlive())
            tracker_.reset();
        }
        break;

      default:
        break;
    }
  }

  return true;
}


void OrthancStone::WebAssemblyViewport::SetupEvents(const std::string& canvas)
{
#if 0
  emscripten_set_click_callback(canvas.c_str(), this, false, OnMouseEvent);
#else
  emscripten_set_mousedown_callback(canvas.c_str(), this, false, OnMouseEvent);
  emscripten_set_mousemove_callback(canvas.c_str(), this, false, OnMouseEvent);
  emscripten_set_mouseup_callback(canvas.c_str(), this, false, OnMouseEvent);
#endif
}

std::auto_ptr<OrthancStone::WebAssemblyViewport>  viewport1_;
std::auto_ptr<OrthancStone::WebAssemblyViewport>  viewport2_;
std::auto_ptr<OrthancStone::WebAssemblyViewport>  viewport3_;

EM_BOOL OnWindowResize(
  int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
  if (viewport1_.get() != NULL)
  {
    viewport1_->UpdateSize();
  }
  
  if (viewport2_.get() != NULL)
  {
    viewport2_->UpdateSize();
  }
  
  if (viewport3_.get() != NULL)
  {
    viewport3_->UpdateSize();
  }
  
  return true;
}

extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    OrthancStone::StoneInitialize();
    // Orthanc::Logging::EnableInfoLevel(true);
    // Orthanc::Logging::EnableTraceLevel(true);
    EM_ASM(window.dispatchEvent(new CustomEvent("WebAssemblyLoaded")););
  }

  EMSCRIPTEN_KEEPALIVE
  void Initialize()
  {
    viewport1_.reset(
      new OrthancStone::WebAssemblyViewport(broker_, "mycanvas1"));
    PrepareScene(viewport1_->GetScene());
    viewport1_->UpdateSize();

    viewport2_.reset(
      new OrthancStone::WebAssemblyViewport(broker_, "mycanvas2"));
    PrepareScene(viewport2_->GetScene());
    viewport2_->UpdateSize();

    viewport3_.reset(
      new OrthancStone::WebAssemblyViewport(broker_, "mycanvas3"));
    PrepareScene(viewport3_->GetScene());
    viewport3_->UpdateSize();

    emscripten_set_resize_callback("#window", NULL, false, OnWindowResize);
  }
}
