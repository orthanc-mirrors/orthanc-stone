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


// From Stone
#include "../../Applications/Sdl/SdlOpenGLWindow.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"

// From Orthanc framework
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>

#include <SDL.h>
#include <stdio.h>

static const unsigned int FONT_SIZE = 64;


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

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
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
  scene.SetLayer(170, new TextSceneLayer(0, 0, "Hello", 0, BitmapAnchor_Center, 20));
}


void TakeScreenshot(const std::string& target,
                    const OrthancStone::Scene2D& scene,
                    unsigned int canvasWidth,
                    unsigned int canvasHeight)
{
  // Take a screenshot, then save it as PNG file
  OrthancStone::CairoCompositor compositor(scene, canvasWidth, canvasHeight);
  compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, FONT_SIZE, Orthanc::Encoding_Latin1);
  compositor.Refresh();

  Orthanc::ImageAccessor canvas;
  compositor.GetCanvas().GetReadOnlyAccessor(canvas);

  Orthanc::Image png(Orthanc::PixelFormat_RGB24, canvas.GetWidth(), canvas.GetHeight(), false);
  Orthanc::ImageProcessing::Convert(png, canvas);
        
  Orthanc::PngWriter writer;
  writer.WriteToFile(target, png);
}


void HandleApplicationEvent(OrthancStone::Scene2D& scene,
                            const SDL_Event& event,
                            std::auto_ptr<OrthancStone::IPointerTracker>& activeTracker,
                            unsigned int windowWidth,
                            unsigned int windowHeight)
{
  if (event.type == SDL_MOUSEBUTTONDOWN)
  {
    OrthancStone::PointerEvent e;
    e.AddIntegerPosition(event.button.x, event.button.y);

    switch (event.button.button)
    {
      case SDL_BUTTON_MIDDLE:
        activeTracker.reset(new OrthancStone::PanSceneTracker(scene, e));
        break;

      case SDL_BUTTON_RIGHT:
        activeTracker.reset(new OrthancStone::ZoomSceneTracker
                            (scene, e, windowWidth, windowHeight));
        break;

      case SDL_BUTTON_LEFT:
        activeTracker.reset(new OrthancStone::RotateSceneTracker
                            (scene, e, windowWidth, windowHeight));
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
        scene.FitContent(windowWidth, windowHeight);
        break;
              
      case SDLK_c:
        TakeScreenshot("screenshot.png", scene, windowWidth, windowHeight);
        break;
              
      default:
        break;
    }
  }
}


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


void Run(OrthancStone::Scene2D& scene)
{
  OrthancStone::SdlOpenGLWindow window("Hello", 1024, 768);
  scene.FitContent(window.GetCanvasWidth(), window.GetCanvasHeight());

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(OpenGLMessageCallback, 0 );

  OrthancStone::OpenGLCompositor compositor(window, scene);
  compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, 
                     FONT_SIZE, Orthanc::Encoding_Latin1);

  std::auto_ptr<OrthancStone::IPointerTracker>  tracker;

  bool stop = false;
  while (!stop)
  {
    compositor.Refresh();

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
        if (tracker.get() != NULL)
        {
          OrthancStone::PointerEvent e;
          e.AddIntegerPosition(event.button.x, event.button.y);
          tracker->Update(e);
        }
      }
      else if (event.type == SDL_MOUSEBUTTONUP)
      {
        if (tracker.get() != NULL)
        {
          tracker->Release();
          tracker.reset(NULL);
        }
      }
      else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      {
        tracker.reset(NULL);
        compositor.UpdateSize();
      }
      else if (event.type == SDL_KEYDOWN &&
               event.key.repeat == 0 /* Ignore key bounce */)
      {
        switch (event.key.keysym.sym)
        {
          case SDLK_f:
            window.GetWindow().ToggleMaximize();
            break;
              
          case SDLK_q:
            stop = true;
            break;

          default:
            HandleApplicationEvent(scene, event, tracker, window.GetCanvasWidth(), window.GetCanvasHeight());
            break;
        }
      }
      else
      {
        HandleApplicationEvent(scene, event, tracker, window.GetCanvasWidth(), window.GetCanvasHeight());
      }
    }

    SDL_Delay(1);
  }
}


int main()
{
  Orthanc::Logging::Initialize();
  OrthancStone::SdlWindow::GlobalInitialize();

  try
  {
    OrthancStone::Scene2D scene;
    PrepareScene(scene);
    Run(scene);
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::SdlWindow::GlobalFinalize();
  Orthanc::Logging::Finalize();

  return 0;
}