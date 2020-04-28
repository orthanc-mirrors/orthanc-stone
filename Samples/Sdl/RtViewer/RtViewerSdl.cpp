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
#include "SampleHelpers.h"

#include <Framework/StoneException.h>

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
}
