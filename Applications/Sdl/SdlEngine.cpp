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


#include "SdlEngine.h"

#if ORTHANC_ENABLE_SDL == 1

#include <Core/Logging.h>

#include <SDL.h>

namespace OrthancStone
{
  void SdlEngine::SetSize(unsigned int width,
                          unsigned int height)
  {
    NativeStoneApplicationContext::GlobalMutexLocker locker(context_);
    locker.GetCentralViewport().SetSize(width, height);
    surface_.SetSize(width, height);
  }


  void SdlEngine::RenderFrame()
  {
    if (viewportChanged_)
    {
      NativeStoneApplicationContext::GlobalMutexLocker locker(context_);
      surface_.Render(locker.GetCentralViewport());

      viewportChanged_ = false;
    }
  }


  KeyboardModifiers SdlEngine::GetKeyboardModifiers(const uint8_t* keyboardState,
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


  SdlEngine::SdlEngine(SdlWindow& window,
                       NativeStoneApplicationContext& context,
                       MessageBroker& broker) :
    IObserver(broker),
    window_(window),
    context_(context),
    surface_(window),
    viewportChanged_(true)
  {
  }
  

  void SdlEngine::Run()
  {
    int scancodeCount = 0;
    const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

    SetSize(window_.GetWidth(), window_.GetHeight());
    
    {
      NativeStoneApplicationContext::GlobalMutexLocker locker(context_);
      locker.GetCentralViewport().FitContent();
    }
    
    bool stop = false;
    while (!stop)
    {
      RenderFrame();

      SDL_Event event;

      while (!stop &&
             SDL_PollEvent(&event))
      {
        NativeStoneApplicationContext::GlobalMutexLocker locker(context_);

        if (event.type == SDL_QUIT)
        {
          stop = true;
          break;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
          KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          switch (event.button.button)
          {
          case SDL_BUTTON_LEFT:
            locker.GetCentralViewport().MouseDown(MouseButton_Left, event.button.x, event.button.y, modifiers, std::vector<Touch>());
            break;
            
          case SDL_BUTTON_RIGHT:
            locker.GetCentralViewport().MouseDown(MouseButton_Right, event.button.x, event.button.y, modifiers, std::vector<Touch>());
            break;
            
          case SDL_BUTTON_MIDDLE:
            locker.GetCentralViewport().MouseDown(MouseButton_Middle, event.button.x, event.button.y, modifiers, std::vector<Touch>());
            break;

          default:
            break;
          }
        }
        else if (event.type == SDL_MOUSEMOTION)
        {
          locker.GetCentralViewport().MouseMove(event.button.x, event.button.y, std::vector<Touch>());
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
          locker.GetCentralViewport().MouseUp();
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
          switch (event.window.event)
          {
          case SDL_WINDOWEVENT_LEAVE:
            locker.GetCentralViewport().MouseLeave();
            break;

          case SDL_WINDOWEVENT_ENTER:
            locker.GetCentralViewport().MouseEnter();
            break;

          case SDL_WINDOWEVENT_SIZE_CHANGED:
            SetSize(event.window.data1, event.window.data2);
            break;

          default:
            break;
          }
        }
        else if (event.type == SDL_MOUSEWHEEL)
        {
          KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          int x, y;
          SDL_GetMouseState(&x, &y);

          if (event.wheel.y > 0)
          {
            locker.GetCentralViewport().MouseWheel(MouseWheelDirection_Up, x, y, modifiers);
          }
          else if (event.wheel.y < 0)
          {
            locker.GetCentralViewport().MouseWheel(MouseWheelDirection_Down, x, y, modifiers);
          }
        }
        else if (event.type == SDL_KEYDOWN &&
                 event.key.repeat == 0 /* Ignore key bounce */)
        {
          KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          switch (event.key.keysym.sym)
          {
          case SDLK_a:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'a', modifiers);  break;
          case SDLK_b:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'b', modifiers);  break;
          case SDLK_c:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'c', modifiers);  break;
          case SDLK_d:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'd', modifiers);  break;
          case SDLK_e:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'e', modifiers);  break;
          case SDLK_f:    window_.ToggleMaximize();                         break;
          case SDLK_g:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'g', modifiers);  break;
          case SDLK_h:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'h', modifiers);  break;
          case SDLK_i:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'i', modifiers);  break;
          case SDLK_j:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'j', modifiers);  break;
          case SDLK_k:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'k', modifiers);  break;
          case SDLK_l:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'l', modifiers);  break;
          case SDLK_m:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'm', modifiers);  break;
          case SDLK_n:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'n', modifiers);  break;
          case SDLK_o:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'o', modifiers);  break;
          case SDLK_p:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'p', modifiers);  break;
          case SDLK_q:    stop = true;                                      break;
          case SDLK_r:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'r', modifiers);  break;
          case SDLK_s:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 's', modifiers);  break;
          case SDLK_t:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 't', modifiers);  break;
          case SDLK_u:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'u', modifiers);  break;
          case SDLK_v:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'v', modifiers);  break;
          case SDLK_w:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'w', modifiers);  break;
          case SDLK_x:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'x', modifiers);  break;
          case SDLK_y:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'y', modifiers);  break;
          case SDLK_z:    locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, 'z', modifiers);  break;
          case SDLK_KP_0: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '0', modifiers);  break;
          case SDLK_KP_1: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '1', modifiers);  break;
          case SDLK_KP_2: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '2', modifiers);  break;
          case SDLK_KP_3: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '3', modifiers);  break;
          case SDLK_KP_4: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '4', modifiers);  break;
          case SDLK_KP_5: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '5', modifiers);  break;
          case SDLK_KP_6: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '6', modifiers);  break;
          case SDLK_KP_7: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '7', modifiers);  break;
          case SDLK_KP_8: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '8', modifiers);  break;
          case SDLK_KP_9: locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '9', modifiers);  break;

          case SDLK_PLUS:
          case SDLK_KP_PLUS:
            locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '+', modifiers);  break;

          case SDLK_MINUS:
          case SDLK_KP_MINUS:
            locker.GetCentralViewport().KeyPressed(KeyboardKeys_Generic, '-', modifiers);  break;

          case SDLK_RIGHT:
            locker.GetCentralViewport().KeyPressed(KeyboardKeys_Right, 0, modifiers);  break;
          case SDLK_LEFT:
            locker.GetCentralViewport().KeyPressed(KeyboardKeys_Left, 0, modifiers);  break;
          case SDLK_UP:
            locker.GetCentralViewport().KeyPressed(KeyboardKeys_Up, 0, modifiers);  break;
          case SDLK_DOWN:
            locker.GetCentralViewport().KeyPressed(KeyboardKeys_Down, 0, modifiers);  break;
          default:
            break;
          }
        }
      }

      // Small delay to avoid using 100% of CPU
      SDL_Delay(1);
    }
  }
}

#endif
