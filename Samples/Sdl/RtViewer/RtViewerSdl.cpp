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

#include "RtViewer.h"
#include "../SdlHelpers.h"
#include "SampleHelpers.h"

#include <Framework/StoneException.h>
#include <Framework/StoneInitialization.h>

#include <Framework/OpenGL/SdlOpenGLContext.h>

#include <boost/program_options.hpp>
#include <SDL.h>

#include <string>

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

namespace OrthancStone
{
  void RtViewerApp::ProcessOptions(int argc, char* argv[])
  {
    namespace po = boost::program_options;
    po::options_description desc("Usage:");

    desc.add_options()
      ("loglevel", po::value<std::string>()->default_value("WARNING"),
       "You can choose WARNING, INFO or TRACE for the logging level: Errors and warnings will always be displayed. (default: WARNING)")

      ("orthanc", po::value<std::string>()->default_value("http://localhost:8042"),
       "Base URL of the Orthanc instance")

      ("ctseries", po::value<std::string>()->default_value("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa"),
       "Orthanc ID of the CT series to load")

      ("rtdose", po::value<std::string>()->default_value("830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb"),
       "Orthanc ID of the RTDOSE instance to load")

      ("rtstruct", po::value<std::string>()->default_value("54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9"),
       "Orthanc ID of the RTSTRUCT instance to load")
      ;

    po::variables_map vm;
    try
    {
      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::notify(vm);
    }
    catch (std::exception& e)
    {
      std::cerr << "Please check your command line options! (\"" << e.what() << "\")" << std::endl;
    }

    if (vm.count("loglevel") > 0)
    {
      std::string logLevel = vm["loglevel"].as<std::string>();
      OrthancStoneHelpers::SetLogLevel(logLevel);
    }

    if (vm.count("orthanc") > 0)
    {
      // maybe check URL validity here
      orthancUrl_ = vm["orthanc"].as<std::string>();
    }

    if (vm.count("ctseries") > 0)
    {
      ctSeriesId_ = vm["ctseries"].as<std::string>();
    }

    if (vm.count("rtdose") > 0)
    {
      doseInstanceId_ = vm["rtdose"].as<std::string>();
    }

    if (vm.count("rtstruct") > 0)
    {
      rtStructInstanceId_ = vm["rtstruct"].as<std::string>();
    }
  }

  void RtViewerApp::RunSdl(int argc, char* argv[])
  {
    ProcessOptions(argc, argv);

    {
      std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
      ViewportController& controller = lock->GetController();
      Scene2D& scene = controller.GetScene();
      ICompositor& compositor = lock->GetCompositor();

      // False means we do NOT let a hi-DPI aware desktop managedr treat this as a legacy application that requires
      // scaling.
      controller.FitContent(compositor.GetCanvasWidth(), compositor.GetCanvasHeight());

      glEnable(GL_DEBUG_OUTPUT);
      glDebugMessageCallback(OpenGLMessageCallback, 0);

      compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_0, Orthanc::Encoding_Latin1);
      compositor.SetFont(1, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_1, Orthanc::Encoding_Latin1);
    }

    /**
    Create the shared loaders context
    */
    loadersContext_.reset(new GenericLoadersContext(1, 4, 1));
    loadersContext_->StartOracle();

    /**
    It is very important that the Oracle (responsible for network I/O) be started before creating and firing the 
    loaders, for any command scheduled by the loader before the oracle is started will be lost.
    */
    PrepareLoadersAndSlicers();

    OrthancStone::DefaultViewportInteractor interactor;

    OrthancStoneHelpers::SdlRunLoop(viewport_, interactor);

    loadersContext_->StopOracle();
  }

#if 0
  void RtViewerApp::HandleApplicationEvent(
    const SDL_Event& event)
  {
    //DisplayInfoText();

    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();
    ICompositor& compositor = lock->GetCompositor();

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
        e.AddPosition(compositor.GetPixelCenterCoordinates(
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
          e.AddPosition(compositor.GetPixelCenterCoordinates(
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
    }
    else if (event.type == SDL_MOUSEBUTTONUP)
    {
      if (activeTracker_)
      {
        PointerEvent e;
        e.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));
        activeTracker_->PointerUp(e);
        if (!activeTracker_->IsAlive())
          activeTracker_.reset();
      }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN)
    {
      PointerEvent e;
      e.AddPosition(compositor.GetPixelCenterCoordinates(
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

      case SDLK_r:
        UpdateLayers();
        {
          std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
          lock->Invalidate();
        }
        break;

      case SDLK_s:
        compositor.FitContent(scene);
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

      case SDLK_z:
        LOG(TRACE) << "SDLK_z has been pressed. event.key.keysym.mod == " << event.key.keysym.mod;
        if (event.key.keysym.mod & KMOD_CTRL)
        {
          if (controller.CanUndo())
          {
            LOG(TRACE) << "Undoing...";
            controller.Undo();
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
          if (controller.CanRedo())
          {
            LOG(TRACE) << "Redoing...";
            controller.Redo();
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
          compositor.GetCanvasWidth(),
          compositor.GetCanvasHeight());
        break;

      default:
        break;
      }
    }
    else if (viewport_->IsRefreshEvent(event))
    {
      // the viewport has been invalidated and requires repaint
      viewport_->Paint();
    }
  }
#endif 

#if 0
  void RtViewerApp::RunSdl(int argc, char* argv[])
  {
    ProcessOptions(argc, argv);

    {
      std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
      ViewportController& controller = lock->GetController();
      Scene2D& scene = controller.GetScene();
      ICompositor& compositor = lock->GetCompositor();

      // False means we do NOT let a hi-DPI aware desktop managedr treat this as a legacy application that requires
      // scaling.
      controller.FitContent(compositor.GetCanvasWidth(), compositor.GetCanvasHeight());

      glEnable(GL_DEBUG_OUTPUT);
      glDebugMessageCallback(OpenGLMessageCallback, 0);

      compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_0, Orthanc::Encoding_Latin1);
      compositor.SetFont(1, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_1, Orthanc::Encoding_Latin1);
    }
                     //////// from loader
    
    loadersContext_.reset(new GenericLoadersContext(1, 4, 1));
    loadersContext_->StartOracle();

    /**
    It is very important that the Oracle (responsible for network I/O) be started before creating and firing the 
    loaders, for any command scheduled by the loader before the oracle is started will be lost.
    */
    PrepareLoadersAndSlicers();

    bool stopApplication = false;

    while (!stopApplication)
    {
      SDL_Event event;
      while (!stopApplication && SDL_PollEvent(&event))
      {
        if (event.type == SDL_QUIT)
        {
          stopApplication = true;
          break;
        }
        else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
          DisableTracker();
        }
        else if (event.type == SDL_KEYDOWN &&
                 event.key.repeat == 0 /* Ignore key bounce */)
        {
          switch (event.key.keysym.sym)
          {
          case SDLK_f:
            // TODO: implement GetWindow to be able to do:
            // viewport_->GetWindow().ToggleMaximize();
            ORTHANC_ASSERT(false, "Please implement GetWindow()");
            break;
          case SDLK_q:
            stopApplication = true;
            break;
          default:
            break;
          }
        }
        // the code above is rather application-neutral.
        // the following call handles events specific to the application
        HandleApplicationEvent(event);
      }
      SDL_Delay(1);
    }
    loadersContext_->StopOracle();
  }
#endif

#if 0
  boost::shared_ptr<IFlexiblePointerTracker> RtViewerApp::CreateSuitableTracker(
    const SDL_Event& event,
    const PointerEvent& e)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();
    ICompositor& compositor = lock->GetCompositor();

    using namespace Orthanc;

    switch (event.button.button)
    {
    case SDL_BUTTON_MIDDLE:
      return boost::shared_ptr<IFlexiblePointerTracker>(new PanSceneTracker
      (viewport_, e));

    case SDL_BUTTON_RIGHT:
      return boost::shared_ptr<IFlexiblePointerTracker>(new ZoomSceneTracker
      (viewport_, e, compositor.GetCanvasHeight()));

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
        case RtViewerGuiTool_Rotate:
          //LOG(TRACE) << "Creating RotateSceneTracker";
          return boost::shared_ptr<IFlexiblePointerTracker>(new RotateSceneTracker(viewport_, e));
        case RtViewerGuiTool_Pan:
          return boost::shared_ptr<IFlexiblePointerTracker>(new PanSceneTracker(viewport_, e));
        case RtViewerGuiTool_Zoom:
          return boost::shared_ptr<IFlexiblePointerTracker>(new ZoomSceneTracker(viewport_, e, compositor.GetCanvasHeight()));
        //case GuiTool_AngleMeasure:
        //  return new AngleMeasureTracker(GetScene(), e);
        //case GuiTool_CircleMeasure:
        //  return new CircleMeasureTracker(GetScene(), e);
        //case GuiTool_EllipseMeasure:
        //  return new EllipseMeasureTracker(GetScene(), e);
        case RtViewerGuiTool_LineMeasure:
          return boost::shared_ptr<IFlexiblePointerTracker>(new CreateLineMeasureTracker(viewport_, e));
        case RtViewerGuiTool_AngleMeasure:
          return boost::shared_ptr<IFlexiblePointerTracker>(new CreateAngleMeasureTracker(viewport_, e));
        case RtViewerGuiTool_CircleMeasure:
          LOG(ERROR) << "Not implemented yet!";
          return boost::shared_ptr<IFlexiblePointerTracker>();
        case RtViewerGuiTool_EllipseMeasure:
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
#endif

}

boost::weak_ptr<OrthancStone::RtViewerApp> g_app;

/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  using namespace OrthancStone;

  StoneInitialize();

  try
  {
    boost::shared_ptr<RtViewerApp> app = RtViewerApp::Create();
    g_app = app;
    app->RunSdl(argc,argv);
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  StoneFinalize();

  return 0;
}

