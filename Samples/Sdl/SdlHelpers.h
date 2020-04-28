#pragma once

#if ORTHANC_ENABLE_SDL != 1
# error This file cannot be used if ORTHANC_ENABLE_SDL != 1
#endif

#include <Framework/Viewport/SdlViewport.h>

#include <boost/shared_ptr.hpp>

#include <SDL.h>

#include <map>
#include <string>

namespace OrthancStoneHelpers
{

  inline OrthancStone::KeyboardModifiers GetKeyboardModifiers(const uint8_t* keyboardState,
                                                              const int scancodeCount)
  {
    using namespace OrthancStone;
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


  inline void GetPointerEvent(OrthancStone::PointerEvent& p,
                              const OrthancStone::ICompositor& compositor,
                              SDL_Event event,
                              const uint8_t* keyboardState,
                              const int scancodeCount)
  {
    using namespace OrthancStone;
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


  inline void SdlRunLoop(boost::shared_ptr<OrthancStone::SdlViewport> viewport,
                         OrthancStone::IViewportInteractor& interactor)
  {
    using namespace OrthancStone;
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
              {
                std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                lock->GetCompositor().FitContent(lock->GetController().GetScene());
                lock->Invalidate();
              }
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
  }




}

