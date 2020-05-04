
#include "SdlSimpleViewerApplication.h"
#include "../SdlHelpers.h"
#include "../../Common/SampleHelpers.h"

#include <Framework/Loaders/GenericLoadersContext.h>
#include <Framework/StoneException.h>
#include <Framework/StoneEnumerations.h>
#include <Framework/StoneInitialization.h>
#include <Framework/Viewport/SdlViewport.h>

#include <Core/OrthancException.h>

#include <boost/program_options.hpp>
#include <SDL.h>

#include <string>


std::string orthancUrl;
std::string instanceId;
int frameIndex = 0;


static void ProcessOptions(int argc, char* argv[])
{
  namespace po = boost::program_options;
  po::options_description desc("Usage:");

  desc.add_options()
    ("loglevel", po::value<std::string>()->default_value("WARNING"),
     "You can choose WARNING, INFO or TRACE for the logging level: Errors and warnings will always be displayed. (default: WARNING)")

    ("orthanc", po::value<std::string>()->default_value("http://localhost:8042"),
     "Base URL of the Orthanc instance")

    ("instance", po::value<std::string>()->default_value("285dece8-e1956b38-cdc7d084-6ce3371e-536a9ffc"),
     "Orthanc ID of the instance to display")

    ("frame_index", po::value<int>()->default_value(0),
     "The zero-based index of the frame (for multi-frame instances)")
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
    orthancUrl = vm["orthanc"].as<std::string>();
  }

  if (vm.count("instance") > 0)
  {
    instanceId = vm["instance"].as<std::string>();
  }

  if (vm.count("frame_index") > 0)
  {
    frameIndex = vm["frame_index"].as<int>();
  }

}

/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  try
  {
    OrthancStone::StoneInitialize();

    ProcessOptions(argc, argv);

    //Orthanc::Logging::EnableInfoLevel(true);
    //Orthanc::Logging::EnableTraceLevel(true);

    {

#if 1
      boost::shared_ptr<OrthancStone::SdlViewport> viewport =
        OrthancStone::SdlOpenGLViewport::Create("Stone of Orthanc", 800, 600);
#else
      boost::shared_ptr<OrthancStone::SdlViewport> viewport =
        OrthancStone::SdlCairoViewport::Create("Stone of Orthanc", 800, 600);
#endif

      OrthancStone::GenericLoadersContext context(1, 4, 1);

      Orthanc::WebServiceParameters orthancWebService;
      orthancWebService.SetUrl(orthancUrl);
      context.SetOrthancParameters(orthancWebService);

      context.StartOracle();

      {

        boost::shared_ptr<SdlSimpleViewerApplication> application(
          SdlSimpleViewerApplication::Create(context, viewport));

        OrthancStone::DicomSource source;

        application->LoadOrthancFrame(source, instanceId, frameIndex);

        OrthancStone::DefaultViewportInteractor interactor;

        {
          int scancodeCount = 0;
          const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

          bool stop = false;
          while (!stop)
          {
            bool paint = false;
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
              if (event.type == SDL_QUIT)
              {
                stop = true;
                break;
              }
              else if (viewport->IsRefreshEvent(event))
              {
                paint = true;
              }
              else if (event.type == SDL_WINDOWEVENT &&
                       (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                        event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
              {
                viewport->UpdateSize(event.window.data1, event.window.data2);
              }
              else if (event.type == SDL_WINDOWEVENT &&
                       (event.window.event == SDL_WINDOWEVENT_SHOWN ||
                        event.window.event == SDL_WINDOWEVENT_EXPOSED))
              {
                paint = true;
              }
              else if (event.type == SDL_KEYDOWN &&
                       event.key.repeat == 0 /* Ignore key bounce */)
              {
                switch (event.key.keysym.sym)
                {
                case SDLK_f:
                  viewport->ToggleMaximize();
                  break;

                case SDLK_s:
                  application->FitContent();
                  break;

                case SDLK_q:
                  stop = true;
                  break;

                default:
                  break;
                }
              }
              else if (event.type == SDL_MOUSEBUTTONDOWN ||
                       event.type == SDL_MOUSEMOTION ||
                       event.type == SDL_MOUSEBUTTONUP)
              {
                std::auto_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                if (lock->HasCompositor())
                {
                  OrthancStone::PointerEvent p;
                  OrthancStoneHelpers::GetPointerEvent(p, lock->GetCompositor(),
                                                       event, keyboardState, scancodeCount);

                  switch (event.type)
                  {
                  case SDL_MOUSEBUTTONDOWN:
                    lock->GetController().HandleMousePress(interactor, p,
                                                           lock->GetCompositor().GetCanvasWidth(),
                                                           lock->GetCompositor().GetCanvasHeight());
                    lock->Invalidate();
                    break;

                  case SDL_MOUSEMOTION:
                    if (lock->GetController().HandleMouseMove(p))
                    {
                      lock->Invalidate();
                    }
                    break;

                  case SDL_MOUSEBUTTONUP:
                    lock->GetController().HandleMouseRelease(p);
                    lock->Invalidate();
                    break;

                  default:
                    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
                  }
                }
              }
            }

            if (paint)
            {
              viewport->Paint();
            }

            // Small delay to avoid using 100% of CPU
            SDL_Delay(1);
          }
        }
        context.StopOracle();
      }
    }

    OrthancStone::StoneFinalize();
    return 0;
  }
  catch (Orthanc::OrthancException& e)
  {
    auto test = e.What();
    fprintf(stdout, test);
    LOG(ERROR) << "OrthancException: " << e.What();
    return -1;
  }
  catch (OrthancStone::StoneException& e)
  {
    LOG(ERROR) << "StoneException: " << e.What();
    return -1;
  }
  catch (std::runtime_error& e)
  {
    LOG(ERROR) << "Runtime error: " << e.what();
    return -1;
  }
  catch (...)
  {
    LOG(ERROR) << "Native exception";
    return -1;
  }
}
