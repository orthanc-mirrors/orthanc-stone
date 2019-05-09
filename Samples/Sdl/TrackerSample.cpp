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
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/StoneInitialization.h"

// From Orthanc framework
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <SDL.h>
#include <stdio.h>


// to be moved into Stone
#include "../Common/MeasureTrackers.h"
#include "../Common/MeasureCommands.h"

/*
TODO:

- to decouple the trackers from the sample, we need to supply them with
  the scene rather than the app

- in order to do that, we need a GetNextFreeZIndex function (or something 
  along those lines) in the scene object

*/


using namespace Orthanc;
using namespace OrthancStone;

namespace OrthancStone
{
  enum GuiTool
  {
    GuiTool_Rotate = 0,
    GuiTool_Pan,
    GuiTool_Zoom,
    GuiTool_LineMeasure,
    GuiTool_CircleMeasure,
    GuiTool_AngleMeasure,
    GuiTool_EllipseMeasure,
    GuiTool_LAST
  };

  const char* MeasureToolToString(size_t i)
  {
    static const char* descs[] = {
      "GuiTool_Rotate",
      "GuiTool_Pan",
      "GuiTool_Zoom",
      "GuiTool_LineMeasure",
      "GuiTool_CircleMeasure",
      "GuiTool_AngleMeasure",
      "GuiTool_EllipseMeasure",
      "GuiTool_LAST"
    };
    if (i >= GuiTool_LAST)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError, "Wrong tool index");
    }
    return descs[i];
  }
}

class TrackerSampleApp
{
public:
  // 12 because.
  TrackerSampleApp() : currentTool_(GuiTool_Rotate)
  {
    TEXTURE_2x2_1_ZINDEX  = 1;
    TEXTURE_1x1_ZINDEX    = 2;
    TEXTURE_2x2_2_ZINDEX  = 3;
    LINESET_1_ZINDEX      = 4;
    LINESET_2_ZINDEX      = 5;
    INFOTEXT_LAYER_ZINDEX = 6;
  }
  void PrepareScene();
  void Run();

private:
  Scene2D& GetScene()
  {
    return scene_;
  }

  void SelectNextTool()
  {
    currentTool_ = static_cast<GuiTool>(currentTool_ + 1);
    if (currentTool_ == GuiTool_LAST)
      currentTool_ = static_cast<GuiTool>(0);;
    printf("Current tool is now: %s\n", MeasureToolToString(currentTool_));
  }

  void HandleApplicationEvent(
    const OpenGLCompositor& compositor,
    const SDL_Event& event,
    std::auto_ptr<IPointerTracker>& activeTracker);

  IPointerTracker* TrackerSampleApp::TrackerHitTest(const PointerEvent& e);

  IPointerTracker* CreateSuitableTracker(
    const SDL_Event& event,
    const PointerEvent& e,
    const OpenGLCompositor& compositor);
  
  void TakeScreenshot(
    const std::string& target,
    unsigned int canvasWidth,
    unsigned int canvasHeight);

  /**
    This adds the command at the top of the undo stack  
  */
  void Commit(TrackerCommandPtr cmd);
  void Undo();
  void Redo();
  
private:
  static const unsigned int FONT_SIZE = 32;

  std::vector<TrackerCommandPtr> undoStack_;
  
  // we store the measure tools here so that they don't get deleted
  std::vector<MeasureToolPtr> measureTools_;

  //static const int LAYER_POSITION = 150;
#if 0
  int TEXTURE_2x2_1_ZINDEX = 12;
  int TEXTURE_1x1_ZINDEX = 13;
  int TEXTURE_2x2_2_ZINDEX = 14;
  int LINESET_1_ZINDEX = 50;
  int LINESET_2_ZINDEX = 100;
  int INFOTEXT_LAYER_ZINDEX = 150;
#else
  int TEXTURE_2x2_1_ZINDEX;
  int TEXTURE_1x1_ZINDEX;
  int TEXTURE_2x2_2_ZINDEX;
  int LINESET_1_ZINDEX;
  int LINESET_2_ZINDEX;
  int INFOTEXT_LAYER_ZINDEX;
#endif
  Scene2D scene_;
  GuiTool currentTool_;
};


void TrackerSampleApp::PrepareScene()
{
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

    scene_.SetLayer(TEXTURE_2x2_1_ZINDEX, new ColorTextureSceneLayer(i));

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-3, 2);
    l->SetPixelSpacing(1.5, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene_.SetLayer(TEXTURE_2x2_2_ZINDEX, l.release());
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
    scene_.SetLayer(TEXTURE_1x1_ZINDEX, l.release());
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

    layer->SetColor(0, 255, 255);
    scene_.SetLayer(LINESET_1_ZINDEX, layer.release());
  }

  // Some text
  {
    std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
    layer->SetText("Hello");
    scene_.SetLayer(LINESET_2_ZINDEX, layer.release());
  }
}


void TrackerSampleApp::TakeScreenshot(const std::string& target,
  unsigned int canvasWidth,
  unsigned int canvasHeight)
{
  // Take a screenshot, then save it as PNG file
  CairoCompositor compositor(scene_, canvasWidth, canvasHeight);
  compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, FONT_SIZE, Orthanc::Encoding_Latin1);
  compositor.Refresh();

  Orthanc::ImageAccessor canvas;
  compositor.GetCanvas().GetReadOnlyAccessor(canvas);

  Orthanc::Image png(Orthanc::PixelFormat_RGB24, canvas.GetWidth(), canvas.GetHeight(), false);
  Orthanc::ImageProcessing::Convert(png, canvas);

  Orthanc::PngWriter writer;
  writer.WriteToFile(target, png);
}


IPointerTracker* TrackerSampleApp::TrackerHitTest(const PointerEvent& e)
{
  // std::vector<MeasureToolPtr> measureTools_;
  return nullptr;
}

IPointerTracker* TrackerSampleApp::CreateSuitableTracker(
  const SDL_Event& event, 
  const PointerEvent& e, 
  const OpenGLCompositor& compositor)
{
  switch (event.button.button)
  {
  case SDL_BUTTON_MIDDLE:
    return new PanSceneTracker(scene_, e);

  case SDL_BUTTON_RIGHT:
    return new ZoomSceneTracker(
      scene_, e, compositor.GetCanvasHeight());

  case SDL_BUTTON_LEFT:
  {
    // TODO: we need to iterate on the set of measuring tool and perform
    // a hit test to check if a tracker needs to be created for edition.
    // Otherwise, depending upon the active tool, we might want to create
    // a "measuring tool creation" tracker

    // TODO: if there are conflicts, we should prefer a tracker that 
    // pertains to the type of measuring tool currently selected (TBD?)
    IPointerTracker* hitTestTracker = TrackerHitTest(e);
    
    if (hitTestTracker != NULL)
    {
      return hitTestTracker;
    }
    else
    { 
      switch (currentTool_)
      {
      case GuiTool_Rotate:
        return new RotateSceneTracker(scene_, e);
      case GuiTool_LineMeasure:
        return new CreateLineMeasureTracker(
          scene_, undoStack_, measureTools_, e);
        //case GuiTool_AngleMeasure:
        //  return new AngleMeasureTracker(scene_, measureTools_, undoStack_, e);
        //case GuiTool_CircleMeasure:
        //  return new CircleMeasureTracker(scene_, measureTools_, undoStack_, e);
        //case GuiTool_EllipseMeasure:
        //  return new EllipseMeasureTracker(scene_, measureTools_, undoStack_, e);
      default:
        throw OrthancException(ErrorCode_InternalError, "Wrong tool!");
      }
    }
  }
  default:
    return NULL;
  }
}

void TrackerSampleApp::HandleApplicationEvent(
  const OpenGLCompositor& compositor,
  const SDL_Event& event,
  std::auto_ptr<IPointerTracker>& activeTracker)
{
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
      e.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));

      ScenePoint2D p = e.GetMainPosition().Apply(scene_.GetCanvasToSceneTransform());

      char buf[64];
      sprintf(buf, "(%0.02f,%0.02f)", p.GetX(), p.GetY());

      if (scene_.HasLayer(INFOTEXT_LAYER_ZINDEX))
      {
        TextSceneLayer& layer =
          dynamic_cast<TextSceneLayer&>(scene_.GetLayer(INFOTEXT_LAYER_ZINDEX));
        layer.SetText(buf);
        layer.SetPosition(p.GetX(), p.GetY());
      }
      else
      {
        std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
        layer->SetColor(0, 255, 0);
        layer->SetText(buf);
        layer->SetBorder(20);
        layer->SetAnchor(BitmapAnchor_BottomCenter);
        layer->SetPosition(p.GetX(), p.GetY());
        scene_.SetLayer(INFOTEXT_LAYER_ZINDEX, layer.release());
      }
    }
    else
    {
      scene_.DeleteLayer(INFOTEXT_LAYER_ZINDEX);
    }
  }
  else if (event.type == SDL_MOUSEBUTTONDOWN)
  {
    PointerEvent e;
    e.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));

    activeTracker.reset(CreateSuitableTracker(event, e, compositor));
  }
  else if (event.type == SDL_KEYDOWN &&
    event.key.repeat == 0 /* Ignore key bounce */)
  {
    switch (event.key.keysym.sym)
    {
    case SDLK_s:
      scene_.FitContent(compositor.GetCanvasWidth(),
        compositor.GetCanvasHeight());
      break;

    case SDLK_c:
      TakeScreenshot(
        "screenshot.png",
        compositor.GetCanvasWidth(),
        compositor.GetCanvasHeight());
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
  const void* userParam)
{
  if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
      (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
      type, severity, message);
  }
}

bool g_stopApplication = false;

void TrackerSampleApp::Run()
{
  SdlOpenGLWindow window("Hello", 1024, 768);

  scene_.FitContent(window.GetCanvasWidth(), window.GetCanvasHeight());

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(OpenGLMessageCallback, 0);

  OpenGLCompositor compositor(window, scene_);
  compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
    FONT_SIZE, Orthanc::Encoding_Latin1);

  // this will either be empty or contain the current tracker, if any
  std::auto_ptr<IPointerTracker>  tracker;

 
  while (!g_stopApplication)
  {
    compositor.Refresh();

    SDL_Event event;
    while (!g_stopApplication && SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        g_stopApplication = true;
        break;
      }
      else if (event.type == SDL_MOUSEMOTION)
      {
        if (tracker.get() != NULL)
        {
          PointerEvent e;
          e.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));
          LOG(TRACE) << "event.button.x = " << event.button.x << "     " <<
            "event.button.y = " << event.button.y;
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
          g_stopApplication = true;
          break;

        case SDLK_t:
          SelectNextTool();
          break;

        default:
          break;
        }
      }
      HandleApplicationEvent(compositor, event, tracker);
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
  StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);

  try
  {
    TrackerSampleApp app;
    app.PrepareScene();
    app.Run();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  StoneFinalize();

  return 0;
}
