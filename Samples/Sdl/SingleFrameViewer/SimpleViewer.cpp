#error TO BE DELETED

#include "SdlSimpleViewerApplication.h"

#include <Core/OrthancException.h>

#include <Framework/Loaders/GenericLoadersContext.h>
#include <Framework/StoneException.h>
#include <Framework/StoneEnumerations.h>
#include <Framework/StoneInitialization.h>
#include <Framework/Viewport/SdlViewport.h>

#include <SDL.h>

namespace OrthancStone
{
  static KeyboardModifiers GetKeyboardModifiers(const uint8_t* keyboardState,
                                                const int scancodeCount)
  {
    int result = KeyboardModifiers_None;

    if (keyboardState != NULL)
    {
      if (SDL_SCANCODE_LSHIFT < scancodeCount &&
          keyboardState[SDL_SCANCODE_LSHIFT])
      {
        result |= KeyboardModifiers_Shift;
      }

      if (SDL_SCANCODE_RSHIFT < scancodeCount &&
          keyboardState[SDL_SCANCODE_RSHIFT])
      {
        result |= KeyboardModifiers_Shift;
      }

      if (SDL_SCANCODE_LCTRL < scancodeCount &&
          keyboardState[SDL_SCANCODE_LCTRL])
      {
        result |= KeyboardModifiers_Control;
      }

      if (SDL_SCANCODE_RCTRL < scancodeCount &&
          keyboardState[SDL_SCANCODE_RCTRL])
      {
        result |= KeyboardModifiers_Control;
      }

      if (SDL_SCANCODE_LALT < scancodeCount &&
          keyboardState[SDL_SCANCODE_LALT])
      {
        result |= KeyboardModifiers_Alt;
      }

      if (SDL_SCANCODE_RALT < scancodeCount &&
          keyboardState[SDL_SCANCODE_RALT])
      {
        result |= KeyboardModifiers_Alt;
      }
    }

    return static_cast<KeyboardModifiers>(result);
  }


  static void GetPointerEvent(PointerEvent& p,
                              const ICompositor& compositor,
                              SDL_Event event,
                              const uint8_t* keyboardState,
                              const int scancodeCount)
  {
    KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

    switch (event.button.button)
    {
    case SDL_BUTTON_LEFT:
      p.SetMouseButton(OrthancStone::MouseButton_Left);
      break;

    case SDL_BUTTON_RIGHT:
      p.SetMouseButton(OrthancStone::MouseButton_Right);
      break;

    case SDL_BUTTON_MIDDLE:
      p.SetMouseButton(OrthancStone::MouseButton_Middle);
      break;

    default:
      p.SetMouseButton(OrthancStone::MouseButton_None);
      break;
    }

    p.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));
    p.SetAltModifier(modifiers & KeyboardModifiers_Alt);
    p.SetControlModifier(modifiers & KeyboardModifiers_Control);
    p.SetShiftModifier(modifiers & KeyboardModifiers_Shift);
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
    Orthanc::Logging::EnableInfoLevel(true);
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
      
      context.StartOracle();

      {

        boost::shared_ptr<SdlSimpleViewerApplication> application(
          SdlSimpleViewerApplication::Create(context, viewport));

        OrthancStone::DicomSource source;

        // Default and command-line parameters
        const char* instanceId = "285dece8-e1956b38-cdc7d084-6ce3371e-536a9ffc";
        unsigned int frameIndex = 0;

        if (argc == 1)
        {
          LOG(ERROR) << "No instanceId supplied. The default of " << instanceId << " will be used. "
            << "Please supply the Orthanc instance ID of the frame you wish to display then, optionally, "
            << "the zero-based index of the frame (for multi-frame instances)";
          // TODO: frame number as second argument...
        }

        if (argc >= 2)
          instanceId = argv[1];

        if (argc >= 3)
          frameIndex = atoi(argv[1]);

        if (argc > 3)
        {
          LOG(ERROR) << "Extra arguments ignored!";
        }
         

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
                  OrthancStone::GetPointerEvent(p, lock->GetCompositor(),
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
  catch (Orthanc::OrthancException & e)
  {
    auto test = e.What();
    fprintf(stdout, test);
    LOG(ERROR) << "OrthancException: " << e.What();
    return -1;
  }
  catch (OrthancStone::StoneException & e)
  {
    LOG(ERROR) << "StoneException: " << e.What();
    return -1;
  }
  catch (std::runtime_error & e)
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
