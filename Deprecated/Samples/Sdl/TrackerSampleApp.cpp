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

#include "TrackerSampleApp.h"

#include "../../Framework/OpenGL/SdlOpenGLContext.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2DViewport/UndoStack.h"
#include "../../Framework/Scene2DViewport/CreateAngleMeasureTracker.h"
#include "../../Framework/Scene2DViewport/CreateLineMeasureTracker.h"
#include "../../Framework/StoneInitialization.h"

// From Orthanc framework
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>

#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <SDL.h>

#include <stdio.h>

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
    std::stringstream msg;
	
	for (std::map<std::string, std::string>::const_iterator kv = infoTextMap_.begin();
		kv != infoTextMap_.end(); ++kv)
    {
      msg << kv->first << " : " << kv->second << std::endl;
    }
	std::string msgS = msg.str();

    TextSceneLayer* layerP = NULL;
    if (controller_->GetScene().HasLayer(FIXED_INFOTEXT_LAYER_ZINDEX))
    {
      TextSceneLayer& layer = dynamic_cast<TextSceneLayer&>(
        controller_->GetScene().GetLayer(FIXED_INFOTEXT_LAYER_ZINDEX));
      layerP = &layer;
    }
    else
    {
      std::unique_ptr<TextSceneLayer> layer(new TextSceneLayer);
      layerP = layer.get();
      layer->SetColor(0, 255, 0);
      layer->SetFontIndex(1);
      layer->SetBorder(20);
      layer->SetAnchor(BitmapAnchor_TopLeft);
      //layer->SetPosition(0,0);
      controller_->GetScene().SetLayer(FIXED_INFOTEXT_LAYER_ZINDEX, layer.release());
    }
    // position the fixed info text in the upper right corner
    layerP->SetText(msgS.c_str());
    double cX = GetCompositor().GetCanvasWidth() * (-0.5);
    double cY = GetCompositor().GetCanvasHeight() * (-0.5);
    controller_->GetScene().GetCanvasToSceneTransform().Apply(cX,cY);
    layerP->SetPosition(cX, cY);
  }

  void TrackerSampleApp::DisplayFloatingCtrlInfoText(const PointerEvent& e)
  {
    ScenePoint2D p = e.GetMainPosition().Apply(controller_->GetScene().GetCanvasToSceneTransform());

    char buf[128];
    sprintf(buf, "S:(%0.02f,%0.02f) C:(%0.02f,%0.02f)", 
      p.GetX(), p.GetY(), 
      e.GetMainPosition().GetX(), e.GetMainPosition().GetY());

    if (controller_->GetScene().HasLayer(FLOATING_INFOTEXT_LAYER_ZINDEX))
    {
      TextSceneLayer& layer =
        dynamic_cast<TextSceneLayer&>(controller_->GetScene().GetLayer(FLOATING_INFOTEXT_LAYER_ZINDEX));
      layer.SetText(buf);
      layer.SetPosition(p.GetX(), p.GetY());
    }
    else
    {
      std::unique_ptr<TextSceneLayer> layer(new TextSceneLayer);
      layer->SetColor(0, 255, 0);
      layer->SetText(buf);
      layer->SetBorder(20);
      layer->SetAnchor(BitmapAnchor_BottomCenter);
      layer->SetPosition(p.GetX(), p.GetY());
      controller_->GetScene().SetLayer(FLOATING_INFOTEXT_LAYER_ZINDEX, layer.release());
    }
  }

  void TrackerSampleApp::HideInfoText()
  {
    controller_->GetScene().DeleteLayer(FLOATING_INFOTEXT_LAYER_ZINDEX);
  }

  ScenePoint2D TrackerSampleApp::GetRandomPointInScene() const
  {
    unsigned int w = GetCompositor().GetCanvasWidth();
    LOG(TRACE) << "GetCompositor().GetCanvasWidth() = " << 
      GetCompositor().GetCanvasWidth();
    unsigned int h = GetCompositor().GetCanvasHeight();
    LOG(TRACE) << "GetCompositor().GetCanvasHeight() = " << 
      GetCompositor().GetCanvasHeight();

    if ((w >= RAND_MAX) || (h >= RAND_MAX))
      LOG(WARNING) << "Canvas is too big : tools will not be randomly placed";

    int x = rand() % w;
    int y = rand() % h;
    LOG(TRACE) << "random x = " << x << "random y = " << y;

    ScenePoint2D p = controller_->GetViewport().GetPixelCenterCoordinates(x, y);
    LOG(TRACE) << "--> p.GetX() = " << p.GetX() << " p.GetY() = " << p.GetY();

    ScenePoint2D r = p.Apply(controller_->GetScene().GetCanvasToSceneTransform());
    LOG(TRACE) << "--> r.GetX() = " << r.GetX() << " r.GetY() = " << r.GetY();
    return r;
  }

  void TrackerSampleApp::CreateRandomMeasureTool()
  {
    static bool srandCalled = false;
    if (!srandCalled)
    {
      srand(42);
      srandCalled = true;
    }

    int i = rand() % 2;
    LOG(TRACE) << "random i = " << i;
    switch (i)
    {
    case 0:
      // line measure
      {
        boost::shared_ptr<CreateLineMeasureCommand> cmd = 
          boost::make_shared<CreateLineMeasureCommand>(
		  boost::ref(IObserver::GetBroker()),
            controller_,
            GetRandomPointInScene());
        cmd->SetEnd(GetRandomPointInScene());
        controller_->PushCommand(cmd);
      }
      break;
    case 1:
      // angle measure
      {
      boost::shared_ptr<CreateAngleMeasureCommand> cmd =
        boost::make_shared<CreateAngleMeasureCommand>(
          boost::ref(IObserver::GetBroker()),
          controller_,
          GetRandomPointInScene());
        cmd->SetCenter(GetRandomPointInScene());
        cmd->SetSide2End(GetRandomPointInScene());
        controller_->PushCommand(cmd);
      }
      break;
    }
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
        SDL_SCANCODE_LALT < scancodeCount &&
        keyboardState[SDL_SCANCODE_LALT])
      {
        // The "left-ctrl" key is down, while no tracker is present
        // Let's display the info text
        PointerEvent e;
        e.AddPosition(controller_->GetViewport().GetPixelCenterCoordinates(
          event.button.x, event.button.y));

        DisplayFloatingCtrlInfoText(e);
      }
      else if (activeTracker_.get() != NULL)
      {
        HideInfoText();
        //LOG(TRACE) << "(event.type == SDL_MOUSEMOTION)";
        if (activeTracker_.get() != NULL)
        {
          //LOG(TRACE) << "(activeTracker_.get() != NULL)";
          PointerEvent e;
          e.AddPosition(controller_->GetViewport().GetPixelCenterCoordinates(
            event.button.x, event.button.y));
          
          //LOG(TRACE) << "event.button.x = " << event.button.x << "     " <<
          //  "event.button.y = " << event.button.y;
          LOG(TRACE) << "activeTracker_->PointerMove(e); " <<
            e.GetMainPosition().GetX() << " " << e.GetMainPosition().GetY();
          
          activeTracker_->PointerMove(e);
          if (!activeTracker_->IsAlive())
            activeTracker_.reset();
        }
      }
      else
      {
        HideInfoText();

        PointerEvent e;
        e.AddPosition(controller_->GetViewport().GetPixelCenterCoordinates(event.button.x, event.button.y));

        ScenePoint2D scenePos = e.GetMainPosition().Apply(
          controller_->GetScene().GetCanvasToSceneTransform());
        //auto measureTools = GetController()->HitTestMeasureTools(scenePos);
        //LOG(TRACE) << "# of hit tests: " << measureTools.size();
        
        // this returns the collection of measuring tools where hit test is true
        std::vector<boost::shared_ptr<MeasureTool> > measureTools = controller_->HitTestMeasureTools(scenePos);

        // let's refresh the measuring tools highlighted state
        // first let's tag them as "unhighlighted"
        controller_->ResetMeasuringToolsHighlight();

        // then immediately take the first one and ask it to highlight the 
        // measuring tool UI part that is hot
        if (measureTools.size() > 0)
        {
          measureTools[0]->Highlight(scenePos);
        }
      }
    }
    else if (event.type == SDL_MOUSEBUTTONUP)
    {
      if (activeTracker_)
      {
        PointerEvent e;
        e.AddPosition(controller_->GetViewport().GetPixelCenterCoordinates(event.button.x, event.button.y));
        activeTracker_->PointerUp(e);
        if (!activeTracker_->IsAlive())
          activeTracker_.reset();
      }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN)
    {
      PointerEvent e;
      e.AddPosition(controller_->GetViewport().GetPixelCenterCoordinates(
        event.button.x, event.button.y));
      if (activeTracker_)
      {
        activeTracker_->PointerDown(e);
        if (!activeTracker_->IsAlive())
          activeTracker_.reset();
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
          if (!activeTracker_->IsAlive())
            activeTracker_.reset();
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

      case SDLK_m:
        CreateRandomMeasureTool();
        break;
      case SDLK_s:
        controller_->FitContent(GetCompositor().GetCanvasWidth(),
          GetCompositor().GetCanvasHeight());
        break;

      case SDLK_z:
        LOG(TRACE) << "SDLK_z has been pressed. event.key.keysym.mod == " << event.key.keysym.mod;
        if (event.key.keysym.mod & KMOD_CTRL)
        {
          if (controller_->CanUndo())
          {
            LOG(TRACE) << "Undoing...";
            controller_->Undo();
          }
          else
          {
            LOG(WARNING) << "Nothing to undo!!!";
          }
        }
        break;

      case SDLK_y:
        LOG(TRACE) << "SDLK_y has been pressed. event.key.keysym.mod == " << event.key.keysym.mod;
        if (event.key.keysym.mod & KMOD_CTRL)
        {
          if (controller_->CanRedo())
          {
            LOG(TRACE) << "Redoing...";
            controller_->Redo();
          }
          else
          {
            LOG(WARNING) << "Nothing to redo!!!";
          }
        }
        break;

      case SDLK_c:
        TakeScreenshot(
          "screenshot.png",
          GetCompositor().GetCanvasWidth(),
          GetCompositor().GetCanvasHeight());
        break;

      default:
        break;
      }
    }
  }


  void TrackerSampleApp::OnSceneTransformChanged(
    const ViewportController::SceneTransformChanged& message)
  {
    DisplayInfoText();
  }

  boost::shared_ptr<IFlexiblePointerTracker> TrackerSampleApp::CreateSuitableTracker(
    const SDL_Event & event,
    const PointerEvent & e)
  {
    using namespace Orthanc;

    switch (event.button.button)
    {
    case SDL_BUTTON_MIDDLE:
      return boost::shared_ptr<IFlexiblePointerTracker>(new PanSceneTracker
        (controller_, e));

    case SDL_BUTTON_RIGHT:
      return boost::shared_ptr<IFlexiblePointerTracker>(new ZoomSceneTracker
        (controller_, e, GetCompositor().GetCanvasHeight()));

    case SDL_BUTTON_LEFT:
    {
      //LOG(TRACE) << "CreateSuitableTracker: case SDL_BUTTON_LEFT:";
      // TODO: we need to iterate on the set of measuring tool and perform
      // a hit test to check if a tracker needs to be created for edition.
      // Otherwise, depending upon the active tool, we might want to create
      // a "measuring tool creation" tracker

      // TODO: if there are conflicts, we should prefer a tracker that 
      // pertains to the type of measuring tool currently selected (TBD?)
      boost::shared_ptr<IFlexiblePointerTracker> hitTestTracker = TrackerHitTest(e);

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
          return boost::shared_ptr<IFlexiblePointerTracker>(new RotateSceneTracker(
            controller_, e));
        case GuiTool_Pan:
          return boost::shared_ptr<IFlexiblePointerTracker>(new PanSceneTracker(
            controller_, e));
        case GuiTool_Zoom:
          return boost::shared_ptr<IFlexiblePointerTracker>(new ZoomSceneTracker(
            controller_, e, GetCompositor().GetCanvasHeight()));
        //case GuiTool_AngleMeasure:
        //  return new AngleMeasureTracker(GetScene(), e);
        //case GuiTool_CircleMeasure:
        //  return new CircleMeasureTracker(GetScene(), e);
        //case GuiTool_EllipseMeasure:
        //  return new EllipseMeasureTracker(GetScene(), e);
        case GuiTool_LineMeasure:
          return boost::shared_ptr<IFlexiblePointerTracker>(new CreateLineMeasureTracker(
            IObserver::GetBroker(), controller_, e));
        case GuiTool_AngleMeasure:
          return boost::shared_ptr<IFlexiblePointerTracker>(new CreateAngleMeasureTracker(
            IObserver::GetBroker(), controller_, e));
        case GuiTool_CircleMeasure:
          LOG(ERROR) << "Not implemented yet!";
          return boost::shared_ptr<IFlexiblePointerTracker>();
        case GuiTool_EllipseMeasure:
          LOG(ERROR) << "Not implemented yet!";
          return boost::shared_ptr<IFlexiblePointerTracker>();
        default:
          throw OrthancException(ErrorCode_InternalError, "Wrong tool!");
        }
      }
    }
    default:
      return boost::shared_ptr<IFlexiblePointerTracker>();
    }
  }


  TrackerSampleApp::TrackerSampleApp(MessageBroker& broker) : IObserver(broker)
    , currentTool_(GuiTool_Rotate)
    , undoStack_(new UndoStack)
    , viewport_("Hello", 1024, 1024, false) // False means we do NOT let Windows treat this as a legacy application that needs to be scaled
  {
    controller_ = boost::shared_ptr<ViewportController>(
      new ViewportController(undoStack_, broker, viewport_));

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

      controller_->GetScene().SetLayer(TEXTURE_2x2_1_ZINDEX, new ColorTextureSceneLayer(i));

      std::unique_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
      l->SetOrigin(-3, 2);
      l->SetPixelSpacing(1.5, 1);
      l->SetAngle(20.0 / 180.0 * M_PI);
      controller_->GetScene().SetLayer(TEXTURE_2x2_2_ZINDEX, l.release());
    }

    // Texture of 1x1 size
    {
      Orthanc::Image i(Orthanc::PixelFormat_RGB24, 1, 1, false);

      uint8_t* p = reinterpret_cast<uint8_t*>(i.GetRow(0));
      p[0] = 255;
      p[1] = 0;
      p[2] = 0;

      std::unique_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
      l->SetOrigin(-2, 1);
      l->SetAngle(20.0 / 180.0 * M_PI);
      controller_->GetScene().SetLayer(TEXTURE_1x1_ZINDEX, l.release());
    }

    // Some lines
    {
      std::unique_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer);

      layer->SetThickness(1);

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

      controller_->GetScene().SetLayer(LINESET_1_ZINDEX, layer.release());
    }

    // Some text
    {
      std::unique_ptr<TextSceneLayer> layer(new TextSceneLayer);
      layer->SetText("Hello");
      controller_->GetScene().SetLayer(LINESET_2_ZINDEX, layer.release());
    }
  }


  void TrackerSampleApp::DisableTracker()
  {
    if (activeTracker_)
    {
      activeTracker_->Cancel();
      activeTracker_.reset();
    }
  }

  void TrackerSampleApp::TakeScreenshot(const std::string& target,
    unsigned int canvasWidth,
    unsigned int canvasHeight)
  {
    CairoCompositor compositor(controller_->GetScene(), canvasWidth, canvasHeight);
    compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, FONT_SIZE_0, Orthanc::Encoding_Latin1);
    compositor.Refresh();

    Orthanc::ImageAccessor canvas;
    compositor.GetCanvas().GetReadOnlyAccessor(canvas);

    Orthanc::Image png(Orthanc::PixelFormat_RGB24, canvas.GetWidth(), canvas.GetHeight(), false);
    Orthanc::ImageProcessing::Convert(png, canvas);

    Orthanc::PngWriter writer;
    writer.WriteToFile(target, png);
  }


  boost::shared_ptr<IFlexiblePointerTracker> TrackerSampleApp::TrackerHitTest(const PointerEvent & e)
  {
    // std::vector<boost::shared_ptr<MeasureTool>> measureTools_;
    ScenePoint2D scenePos = e.GetMainPosition().Apply(
      controller_->GetScene().GetCanvasToSceneTransform());

    std::vector<boost::shared_ptr<MeasureTool> > measureTools = controller_->HitTestMeasureTools(scenePos);

    if (measureTools.size() > 0)
    {
      return measureTools[0]->CreateEditionTracker(e);
    }
    return boost::shared_ptr<IFlexiblePointerTracker>();
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
  
  ICompositor& TrackerSampleApp::GetCompositor()
  {
    using namespace Orthanc;
    try
    {
      SdlViewport& viewport = dynamic_cast<SdlViewport&>(viewport_);
      return viewport.GetCompositor();
    }
    catch (std::bad_cast e)
    {
      throw OrthancException(ErrorCode_InternalError, "Wrong viewport type!");
     }
  }

  const ICompositor& TrackerSampleApp::GetCompositor() const
  {
    using namespace Orthanc;
    try
    {
      SdlViewport& viewport = const_cast<SdlViewport&>(dynamic_cast<const SdlViewport&>(viewport_));
      return viewport.GetCompositor();
    }
    catch (std::bad_cast e)
    {
      throw OrthancException(ErrorCode_InternalError, "Wrong viewport type!");
    }
  }


  void TrackerSampleApp::Run()
  {
    controller_->FitContent(viewport_.GetCanvasWidth(), viewport_.GetCanvasHeight());

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(OpenGLMessageCallback, 0);

    GetCompositor().SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
      FONT_SIZE_0, Orthanc::Encoding_Latin1);
    GetCompositor().SetFont(1, Orthanc::EmbeddedResources::UBUNTU_FONT,
      FONT_SIZE_1, Orthanc::Encoding_Latin1);

    while (!g_stopApplication)
    {
      GetCompositor().Refresh();

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
        }
        else if (event.type == SDL_KEYDOWN &&
          event.key.repeat == 0 /* Ignore key bounce */)
        {
          switch (event.key.keysym.sym)
          {
          case SDLK_f:
            viewport_.GetWindow().ToggleMaximize();
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