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

#include "TrackerSampleApp.h"

#include <Framework/Scene2DViewport/CreateLineMeasureTracker.h>
#include <Framework/Scene2DViewport/CreateAngleMeasureTracker.h>

#include <Framework/Scene2D/PanSceneTracker.h>
#include <Framework/Scene2D/RotateSceneTracker.h>
#include <Framework/Scene2D/Scene2D.h>
#include <Framework/Scene2D/ZoomSceneTracker.h>
#include <Framework/Scene2D/CairoCompositor.h>
#include <Framework/Scene2D/ColorTextureSceneLayer.h>
#include <Framework/Scene2D/OpenGLCompositor.h>

#include <Framework/StoneInitialization.h>

#include <Applications/Sdl/SdlOpenGLWindow.h>

// From Orthanc framework
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>

#include <SDL.h>
#include <stdio.h>

using namespace Orthanc;

namespace OrthancStone
{
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

  Scene2DPtr TrackerSampleApp::GetScene()
  {
    return controller_.GetScene();
  }

  void TrackerSampleApp::SelectNextTool()
  {
    currentTool_ = static_cast<GuiTool>(currentTool_ + 1);
    if (currentTool_ == GuiTool_LAST)
      currentTool_ = static_cast<GuiTool>(0);;
    printf("Current tool is now: %s\n", MeasureToolToString(currentTool_));
  }

  void TrackerSampleApp::DisplayInfoText()
  {
    // do not try to use stuff too early!
    if (compositor_.get() == NULL)
      return;

    std::stringstream msg;
    for (auto kv : infoTextMap_)
    {
      msg << kv.first << " : " << kv.second << std::endl;
    }
    auto msgS = msg.str();

    TextSceneLayer* layerP = NULL;
    if (GetScene()->HasLayer(FIXED_INFOTEXT_LAYER_ZINDEX))
    {
      TextSceneLayer& layer = dynamic_cast<TextSceneLayer&>(
        GetScene()->GetLayer(FIXED_INFOTEXT_LAYER_ZINDEX));
      layerP = &layer;
    }
    else
    {
      std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
      layerP = layer.get();
      layer->SetColor(0, 255, 0);
      layer->SetFontIndex(1);
      layer->SetBorder(20);
      layer->SetAnchor(BitmapAnchor_TopLeft);
      //layer->SetPosition(0,0);
      GetScene()->SetLayer(FIXED_INFOTEXT_LAYER_ZINDEX, layer.release());
    }
    // position the fixed info text in the upper right corner
    layerP->SetText(msgS.c_str());
    double cX = compositor_->GetCanvasWidth() * (-0.5);
    double cY = compositor_->GetCanvasHeight() * (-0.5);
    GetScene()->GetCanvasToSceneTransform().Apply(cX,cY);
    layerP->SetPosition(cX, cY);
  }

  void TrackerSampleApp::DisplayFloatingCtrlInfoText(const PointerEvent& e)
  {
    ScenePoint2D p = e.GetMainPosition().Apply(GetScene()->GetCanvasToSceneTransform());

    char buf[128];
    sprintf(buf, "S:(%0.02f,%0.02f) C:(%0.02f,%0.02f)", 
      p.GetX(), p.GetY(), 
      e.GetMainPosition().GetX(), e.GetMainPosition().GetY());

    if (GetScene()->HasLayer(FLOATING_INFOTEXT_LAYER_ZINDEX))
    {
      TextSceneLayer& layer =
        dynamic_cast<TextSceneLayer&>(GetScene()->GetLayer(FLOATING_INFOTEXT_LAYER_ZINDEX));
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
      GetScene()->SetLayer(FLOATING_INFOTEXT_LAYER_ZINDEX, layer.release());
    }
  }

  void TrackerSampleApp::HideInfoText()
  {
    GetScene()->DeleteLayer(FLOATING_INFOTEXT_LAYER_ZINDEX);
  }

  void TrackerSampleApp::HandleApplicationEvent(
    const SDL_Event & event)
  {
    DisplayInfoText();

    if (event.type == SDL_MOUSEMOTION)
    {
      int scancodeCount = 0;
      const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

      if (activeTracker_.get() == NULL &&
        SDL_SCANCODE_LCTRL < scancodeCount &&
        keyboardState[SDL_SCANCODE_LCTRL])
      {
        // The "left-ctrl" key is down, while no tracker is present
        // Let's display the info text
        PointerEvent e;
        e.AddPosition(compositor_->GetPixelCenterCoordinates(
          event.button.x, event.button.y));

        DisplayFloatingCtrlInfoText(e);
      }
      else
      {
        HideInfoText();
        //LOG(TRACE) << "(event.type == SDL_MOUSEMOTION)";
        if (activeTracker_.get() != NULL)
        {
          //LOG(TRACE) << "(activeTracker_.get() != NULL)";
          PointerEvent e;
          e.AddPosition(compositor_->GetPixelCenterCoordinates(
            event.button.x, event.button.y));
          
          //LOG(TRACE) << "event.button.x = " << event.button.x << "     " <<
          //  "event.button.y = " << event.button.y;
          //LOG(TRACE) << "activeTracker_->PointerMove(e); " <<
          //  e.GetMainPosition().GetX() << " " << e.GetMainPosition().GetY();
          
          activeTracker_->PointerMove(e);
          if (!activeTracker_->IsActive())
            activeTracker_ = NULL;
        }
      }
    }
    else if (event.type == SDL_MOUSEBUTTONUP)
    {
      if (activeTracker_)
      {
        PointerEvent e;
        e.AddPosition(compositor_->GetPixelCenterCoordinates(event.button.x, event.button.y));
        activeTracker_->PointerUp(e);
        if (!activeTracker_->IsActive())
          activeTracker_ = NULL;
      }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN)
    {
      PointerEvent e;
      e.AddPosition(compositor_->GetPixelCenterCoordinates(
        event.button.x, event.button.y));
      if (activeTracker_)
      {
        activeTracker_->PointerDown(e);
        if (!activeTracker_->IsActive())
          activeTracker_ = NULL;
      }
      else
      {
        // we ATTEMPT to create a tracker if need be
        activeTracker_ = CreateSuitableTracker(event, e);
      }
    }
    else if (event.type == SDL_KEYDOWN &&
      event.key.repeat == 0 /* Ignore key bounce */)
    {
      switch (event.key.keysym.sym)
      {
      case SDLK_ESCAPE:
        if (activeTracker_)
        {
          activeTracker_->Cancel();
          if (!activeTracker_->IsActive())
            activeTracker_ = NULL;
        }
        break;

      case SDLK_t:
        if (!activeTracker_)
          SelectNextTool();
        else
        {
          LOG(WARNING) << "You cannot change the active tool when an interaction"
            " is taking place";
        }
        break;

      case SDLK_s:
        GetScene()->FitContent(compositor_->GetCanvasWidth(),
          compositor_->GetCanvasHeight());
        break;

      case SDLK_c:
        TakeScreenshot(
          "screenshot.png",
          compositor_->GetCanvasWidth(),
          compositor_->GetCanvasHeight());
        break;

      default:
        break;
      }
    }
  }


  void TrackerSampleApp::OnSceneTransformChanged(const Scene2D::SceneTransformChanged& message)
  {
    DisplayInfoText();
  }

  FlexiblePointerTrackerPtr TrackerSampleApp::CreateSuitableTracker(
    const SDL_Event & event,
    const PointerEvent & e)
  {
    switch (event.button.button)
    {
    case SDL_BUTTON_MIDDLE:
      return CreateSimpleTrackerAdapter(PointerTrackerPtr(
        new PanSceneTracker(*GetScene(), e)));

    case SDL_BUTTON_RIGHT:
      return CreateSimpleTrackerAdapter(PointerTrackerPtr(
        new ZoomSceneTracker(*GetScene(), e, compositor_->GetCanvasHeight())));

    case SDL_BUTTON_LEFT:
    {
      //LOG(TRACE) << "CreateSuitableTracker: case SDL_BUTTON_LEFT:";
      // TODO: we need to iterate on the set of measuring tool and perform
      // a hit test to check if a tracker needs to be created for edition.
      // Otherwise, depending upon the active tool, we might want to create
      // a "measuring tool creation" tracker

      // TODO: if there are conflicts, we should prefer a tracker that 
      // pertains to the type of measuring tool currently selected (TBD?)
      FlexiblePointerTrackerPtr hitTestTracker = TrackerHitTest(e);

      if (hitTestTracker != NULL)
      {
        //LOG(TRACE) << "hitTestTracker != NULL";
        return hitTestTracker;
      }
      else
      {
        switch (currentTool_)
        {
        case GuiTool_Rotate:
          //LOG(TRACE) << "Creating RotateSceneTracker";
          return CreateSimpleTrackerAdapter(PointerTrackerPtr(
            new RotateSceneTracker(*GetScene(), e)));
        case GuiTool_Pan:
          return CreateSimpleTrackerAdapter(PointerTrackerPtr(
            new PanSceneTracker(*GetScene(), e)));
        case GuiTool_Zoom:
          return CreateSimpleTrackerAdapter(PointerTrackerPtr(
            new ZoomSceneTracker(*GetScene(), e, compositor_->GetCanvasHeight())));
        //case GuiTool_AngleMeasure:
        //  return new AngleMeasureTracker(GetScene(), measureTools_, undoStack_, e);
        //case GuiTool_CircleMeasure:
        //  return new CircleMeasureTracker(GetScene(), measureTools_, undoStack_, e);
        //case GuiTool_EllipseMeasure:
        //  return new EllipseMeasureTracker(GetScene(), measureTools_, undoStack_, e);
        case GuiTool_LineMeasure:
          return FlexiblePointerTrackerPtr(new CreateLineMeasureTracker(
            IObserver::GetBroker(), GetScene(), undoStack_, measureTools_, e));
        case GuiTool_AngleMeasure:
          return FlexiblePointerTrackerPtr(new CreateAngleMeasureTracker(
            IObserver::GetBroker(), GetScene(), undoStack_, measureTools_, e));
          return NULL;
        case GuiTool_CircleMeasure:
          LOG(ERROR) << "Not implemented yet!";
          return NULL;
        case GuiTool_EllipseMeasure:
          LOG(ERROR) << "Not implemented yet!";
          return NULL;
        default:
          throw OrthancException(ErrorCode_InternalError, "Wrong tool!");
        }
      }
    }
    default:
      return NULL;
    }
  }


  TrackerSampleApp::TrackerSampleApp(MessageBroker& broker) : IObserver(broker)
    , currentTool_(GuiTool_Rotate)
  {
    controller_ = ViewportControllerPtr(new ViewportController(broker));

    controller_->RegisterObserverCallback(
      new Callable<TrackerSampleApp, ViewportController::SceneTransformChanged>
      (*this, &TrackerSampleApp::OnSceneTransformChanged));

    TEXTURE_2x2_1_ZINDEX = 1;
    TEXTURE_1x1_ZINDEX = 2;
    TEXTURE_2x2_2_ZINDEX = 3;
    LINESET_1_ZINDEX = 4;
    LINESET_2_ZINDEX = 5;
    FLOATING_INFOTEXT_LAYER_ZINDEX = 6;
    FIXED_INFOTEXT_LAYER_ZINDEX = 7;
  }

  void TrackerSampleApp::PrepareScene()
  {
    // Texture of 2x2 size
    {
      Orthanc::Image i(Orthanc::PixelFormat_RGB24, 2, 2, false);

      uint8_t* p = reinterpret_cast<uint8_t*>(i.GetRow(0));
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

      GetScene()->SetLayer(TEXTURE_2x2_1_ZINDEX, new ColorTextureSceneLayer(i));

      std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
      l->SetOrigin(-3, 2);
      l->SetPixelSpacing(1.5, 1);
      l->SetAngle(20.0 / 180.0 * M_PI);
      GetScene()->SetLayer(TEXTURE_2x2_2_ZINDEX, l.release());
    }

    // Texture of 1x1 size
    {
      Orthanc::Image i(Orthanc::PixelFormat_RGB24, 1, 1, false);

      uint8_t* p = reinterpret_cast<uint8_t*>(i.GetRow(0));
      p[0] = 255;
      p[1] = 0;
      p[2] = 0;

      std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
      l->SetOrigin(-2, 1);
      l->SetAngle(20.0 / 180.0 * M_PI);
      GetScene()->SetLayer(TEXTURE_1x1_ZINDEX, l.release());
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
      GetScene()->SetLayer(LINESET_1_ZINDEX, layer.release());
    }

    // Some text
    {
      std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
      layer->SetText("Hello");
      GetScene()->SetLayer(LINESET_2_ZINDEX, layer.release());
    }
  }


  void TrackerSampleApp::DisableTracker()
  {
    if (activeTracker_)
    {
      activeTracker_->Cancel();
      activeTracker_ = NULL;
    }
  }

  void TrackerSampleApp::TakeScreenshot(const std::string& target,
    unsigned int canvasWidth,
    unsigned int canvasHeight)
  {
    CairoCompositor compositor(GetScene(), canvasWidth, canvasHeight);
    compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, FONT_SIZE_0, Orthanc::Encoding_Latin1);
    compositor.Refresh();

    Orthanc::ImageAccessor canvas;
    compositor.GetCanvas().GetReadOnlyAccessor(canvas);

    Orthanc::Image png(Orthanc::PixelFormat_RGB24, canvas.GetWidth(), canvas.GetHeight(), false);
    Orthanc::ImageProcessing::Convert(png, canvas);

    Orthanc::PngWriter writer;
    writer.WriteToFile(target, png);
  }


  FlexiblePointerTrackerPtr TrackerSampleApp::TrackerHitTest(const PointerEvent & e)
  {
    // std::vector<MeasureToolPtr> measureTools_;
    return nullptr;
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

  static bool g_stopApplication = false;
  
  void TrackerSampleApp::Run()
  {
    // False means we do NOT let Windows treat this as a legacy application
    // that needs to be scaled
    SdlOpenGLWindow window("Hello", 1024, 1024, false);

    GetScene()->FitContent(window.GetCanvasWidth(), window.GetCanvasHeight());

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(OpenGLMessageCallback, 0);

    compositor_.reset(new OpenGLCompositor(window, GetScene()));

    compositor_->SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
      FONT_SIZE_0, Orthanc::Encoding_Latin1);
    compositor_->SetFont(1, Orthanc::EmbeddedResources::UBUNTU_FONT,
      FONT_SIZE_1, Orthanc::Encoding_Latin1);

    while (!g_stopApplication)
    {
      compositor_->Refresh();

      SDL_Event event;
      while (!g_stopApplication && SDL_PollEvent(&event))
      {
        if (event.type == SDL_QUIT)
        {
          g_stopApplication = true;
          break;
        }
        else if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
          DisableTracker(); // was: tracker.reset(NULL);
          compositor_->UpdateSize();
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
          default:
            break;
          }
        }
        HandleApplicationEvent(event);
      }
      SDL_Delay(1);
    }
    compositor_.reset(NULL);
  }

  void TrackerSampleApp::SetInfoDisplayMessage(
    std::string key, std::string value)
  {
    if (value == "")
      infoTextMap_.erase(key);
    else
      infoTextMap_[key] = value;
    DisplayInfoText();
  }

}
