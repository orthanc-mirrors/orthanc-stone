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


// From Stone
#include "../../Framework/Viewport/SdlViewport.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2DViewport/ViewportController.h"
#include "../../Framework/Scene2DViewport/UndoStack.h"

#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Messages/MessageBroker.h"

// From Orthanc framework
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>

#include <boost/make_shared.hpp>

#include <SDL.h>
#include <stdio.h>

static const unsigned int FONT_SIZE = 32;
static const int LAYER_POSITION = 150;

#define OPENGL_ENABLED 0

void PrepareScene(OrthancStone::Scene2D& scene)
{
  using namespace OrthancStone;

  // Texture of 2x2 size
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

    std::unique_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-3, 2);
    l->SetPixelSpacing(1.5, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene.SetLayer(14, l.release());
  }

  // Texture of 1x1 size
  {
    Orthanc::Image i(Orthanc::PixelFormat_RGB24, 1, 1, false);
    
    uint8_t *p = reinterpret_cast<uint8_t*>(i.GetRow(0));
    p[0] = 255;
    p[1] = 0;
    p[2] = 0;

    std::unique_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-2, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene.SetLayer(13, l.release());
  }

  // Some lines
  {
    std::unique_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer);

    layer->SetThickness(10);

    PolylineSceneLayer::Chain chain;
    chain.push_back(ScenePoint2D(0 - 0.5, 0 - 0.5));
    chain.push_back(ScenePoint2D(0 - 0.5, 2 - 0.5));
    chain.push_back(ScenePoint2D(2 - 0.5, 2 - 0.5));
    chain.push_back(ScenePoint2D(2 - 0.5, 0 - 0.5));
    layer->AddChain(chain, true, 255, 0, 0);

    chain.clear();
    chain.push_back(ScenePoint2D(-5, -5));
    chain.push_back(ScenePoint2D(5, -5));
    chain.push_back(ScenePoint2D(5, 5));
    chain.push_back(ScenePoint2D(-5, 5));
    layer->AddChain(chain, true, 0, 255, 0);

    double dy = 1.01;
    chain.clear();
    chain.push_back(ScenePoint2D(-4, -4));
    chain.push_back(ScenePoint2D(4, -4 + dy));
    chain.push_back(ScenePoint2D(-4, -4 + 2.0 * dy));
    chain.push_back(ScenePoint2D(4, 2));
    layer->AddChain(chain, false, 0, 0, 255);

    scene.SetLayer(50, layer.release());
  }

  // Some text
  {
    std::unique_ptr<TextSceneLayer> layer(new TextSceneLayer);
    layer->SetText("Hello");
    scene.SetLayer(100, layer.release());
  }
}


void TakeScreenshot(const std::string& target,
                    const OrthancStone::Scene2D& scene,
                    unsigned int canvasWidth,
                    unsigned int canvasHeight)
{
  using namespace OrthancStone;
  // Take a screenshot, then save it as PNG file
  CairoCompositor compositor(scene, canvasWidth, canvasHeight);
  compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, FONT_SIZE, Orthanc::Encoding_Latin1);
  compositor.Refresh();

  Orthanc::ImageAccessor canvas;
  compositor.GetCanvas().GetReadOnlyAccessor(canvas);

  Orthanc::Image png(Orthanc::PixelFormat_RGB24, canvas.GetWidth(), canvas.GetHeight(), false);
  Orthanc::ImageProcessing::Convert(png, canvas);
        
  Orthanc::PngWriter writer;
  writer.WriteToFile(target, png);
}


void HandleApplicationEvent(const SDL_Event& event,
                            boost::shared_ptr<OrthancStone::ViewportController>& controller,
                            boost::shared_ptr<OrthancStone::IFlexiblePointerTracker>& activeTracker)
{
  using namespace OrthancStone;

  Scene2D& scene = controller->GetScene();
  IViewport& viewport = controller->GetViewport();

  if (event.type == SDL_MOUSEMOTION)
  {
    int scancodeCount = 0;
    const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

    if (activeTracker.get() == NULL &&
        SDL_SCANCODE_LCTRL < scancodeCount &&
        keyboardState[SDL_SCANCODE_LCTRL])
    {
      // The "left-ctrl" key is down, while no tracker is present

      PointerEvent e;
      e.AddPosition(viewport.GetPixelCenterCoordinates(event.button.x, event.button.y));

      ScenePoint2D p = e.GetMainPosition().Apply(scene.GetCanvasToSceneTransform());

      char buf[64];
      sprintf(buf, "(%0.02f,%0.02f)", p.GetX(), p.GetY());

      if (scene.HasLayer(LAYER_POSITION))
      {
        TextSceneLayer& layer =
          dynamic_cast<TextSceneLayer&>(scene.GetLayer(LAYER_POSITION));
        layer.SetText(buf);
        layer.SetPosition(p.GetX(), p.GetY());
      }
      else
      {
        std::unique_ptr<TextSceneLayer> 
          layer(new TextSceneLayer);
        layer->SetColor(0, 255, 0);
        layer->SetText(buf);
        layer->SetBorder(20);
        layer->SetAnchor(BitmapAnchor_BottomCenter);
        layer->SetPosition(p.GetX(), p.GetY());
        scene.SetLayer(LAYER_POSITION, layer.release());
      }
    }
    else
    {
      scene.DeleteLayer(LAYER_POSITION);
    }
  }
  else if (event.type == SDL_MOUSEBUTTONDOWN)
  {
    PointerEvent e;
    e.AddPosition(viewport.GetPixelCenterCoordinates(event.button.x, event.button.y));

    switch (event.button.button)
    {
      case SDL_BUTTON_MIDDLE:
        activeTracker = boost::make_shared<PanSceneTracker>(controller, e);
        break;

      case SDL_BUTTON_RIGHT:
        activeTracker = boost::make_shared<ZoomSceneTracker>
          (controller, e, viewport.GetCanvasHeight());
        break;

      case SDL_BUTTON_LEFT:
        activeTracker = boost::make_shared<RotateSceneTracker>(controller, e);
        break;

      default:
        break;
    }
  }
  else if (event.type == SDL_KEYDOWN &&
           event.key.repeat == 0 /* Ignore key bounce */)
  {
    switch (event.key.keysym.sym)
    {
      case SDLK_s:
        controller->FitContent(viewport.GetCanvasWidth(), 
                               viewport.GetCanvasHeight());
        break;
              
      case SDLK_c:
        TakeScreenshot("screenshot.png", scene, 
                       viewport.GetCanvasWidth(), 
                       viewport.GetCanvasHeight());
        break;
              
      default:
        break;
    }
  }
}

#if OPENGL_ENABLED==1
static void GLAPIENTRY
OpenGLMessageCallback(GLenum source,
                      GLenum type,
                      GLuint id,
                      GLenum severity,
                      GLsizei length,
                      const GLchar* message,
                      const void* userParam )
{
  if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
  }
}
#endif

void Run(OrthancStone::MessageBroker& broker,
         OrthancStone::SdlViewport& viewport)
{
  using namespace OrthancStone;
  
  boost::shared_ptr<ViewportController> controller(
    new ViewportController(boost::make_shared<UndoStack>(), broker, viewport));
  
#if OPENGL_ENABLED==1
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(OpenGLMessageCallback, 0);
#endif

  boost::shared_ptr<IFlexiblePointerTracker> tracker;

  bool firstShown = true;
  bool stop = false;
  while (!stop)
  {
    viewport.Refresh();

    SDL_Event event;
    while (!stop &&
           SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        stop = true;
        break;
      }
      else if (event.type == SDL_MOUSEMOTION)
      {
        if (tracker)
        {
          PointerEvent e;
          e.AddPosition(viewport.GetPixelCenterCoordinates(
            event.button.x, event.button.y));
          tracker->PointerMove(e);
        }
      }
      else if (event.type == SDL_MOUSEBUTTONUP)
      {
        if (tracker)
        {
          PointerEvent e;
          e.AddPosition(viewport.GetPixelCenterCoordinates(
            event.button.x, event.button.y));
          tracker->PointerUp(e);
          if(!tracker->IsAlive())
            tracker.reset();
        }
      }
      else if (event.type == SDL_WINDOWEVENT)
      {
        switch (event.window.event)
        {
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            tracker.reset();
            viewport.UpdateSize(event.window.data1, event.window.data2);
            break;

          case SDL_WINDOWEVENT_SHOWN:
            if (firstShown)
            {
              // Once the window is first shown, fit the content to its size
              controller->FitContent(viewport.GetCanvasWidth(), viewport.GetCanvasHeight());
              firstShown = false;
            }
            
            break;

          default:
            break;
        }
      }
      else if (event.type == SDL_KEYDOWN &&
               event.key.repeat == 0 /* Ignore key bounce */)
      {
        switch (event.key.keysym.sym)
        {
          case SDLK_f:
            viewport.GetWindow().ToggleMaximize();
            break;
              
          case SDLK_q:
            stop = true;
            break;

          default:
            break;
        }
      }
      
      HandleApplicationEvent(event, controller, tracker);
    }

    SDL_Delay(1);
  }
}




/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  OrthancStone::StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);

  try
  {
#if OPENGL_ENABLED==1
    OrthancStone::SdlOpenGLViewport viewport("Hello", 1024, 768);
#else
    OrthancStone::SdlCairoViewport viewport("Hello", 1024, 768);
#endif
    PrepareScene(viewport.GetScene());

    viewport.GetCompositor().SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, 
                                     FONT_SIZE, Orthanc::Encoding_Latin1);
    
    OrthancStone::MessageBroker broker;
    Run(broker, viewport);
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::StoneFinalize();

  return 0;
}