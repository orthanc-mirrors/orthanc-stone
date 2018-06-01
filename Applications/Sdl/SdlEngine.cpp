/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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
  void SdlEngine::SetSize(BasicApplicationContext::ViewportLocker& locker,
                          unsigned int width,
                          unsigned int height)
  {
    locker.GetViewport().SetSize(width, height);
    surface_.SetSize(width, height);
  }
    

  void SdlEngine::RenderFrame()
  {
    if (viewportChanged_)
    {
      BasicApplicationContext::ViewportLocker locker(context_);
      surface_.Render(locker.GetViewport());

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
                       BasicApplicationContext& context) :
    window_(window),
    context_(context),
    surface_(window),
    viewportChanged_(true)
  {
  }
  

  SdlEngine::~SdlEngine()
  {
  }


  void SdlEngine::Run()
  {
    int scancodeCount = 0;
    const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

    {
      BasicApplicationContext::ViewportLocker locker(context_);
      SetSize(locker, window_.GetWidth(), window_.GetHeight());
      locker.GetViewport().SetDefaultView();
    }
    
    bool stop = false;
    while (!stop)
    {
      RenderFrame();

      SDL_Event event;

      while (!stop &&
             SDL_PollEvent(&event))
      {
        BasicApplicationContext::ViewportLocker locker(context_);

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
              locker.GetViewport().MouseDown(MouseButton_Left, event.button.x, event.button.y, modifiers);
              break;
            
            case SDL_BUTTON_RIGHT:
              locker.GetViewport().MouseDown(MouseButton_Right, event.button.x, event.button.y, modifiers);
              break;
            
            case SDL_BUTTON_MIDDLE:
              locker.GetViewport().MouseDown(MouseButton_Middle, event.button.x, event.button.y, modifiers);
              break;

            default:
              break;
          }
        }
        else if (event.type == SDL_MOUSEMOTION)
        {
          locker.GetViewport().MouseMove(event.button.x, event.button.y);
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
          locker.GetViewport().MouseUp();
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
          switch (event.window.event)
          {
            case SDL_WINDOWEVENT_LEAVE:
              locker.GetViewport().MouseLeave();
              break;

            case SDL_WINDOWEVENT_ENTER:
              locker.GetViewport().MouseEnter();
              break;

            case SDL_WINDOWEVENT_SIZE_CHANGED:
              SetSize(locker, event.window.data1, event.window.data2);
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
            locker.GetViewport().MouseWheel(MouseWheelDirection_Up, x, y, modifiers);
          }
          else if (event.wheel.y < 0)
          {
            locker.GetViewport().MouseWheel(MouseWheelDirection_Down, x, y, modifiers);
          }
        }
        else if (event.type == SDL_KEYDOWN &&
                 event.key.repeat == 0 /* Ignore key bounce */)
        {
          KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          switch (event.key.keysym.sym)
          {
            case SDLK_a:    locker.GetViewport().KeyPressed('a', modifiers);  break;
            case SDLK_b:    locker.GetViewport().KeyPressed('b', modifiers);  break;
            case SDLK_c:    locker.GetViewport().KeyPressed('c', modifiers);  break;
            case SDLK_d:    locker.GetViewport().KeyPressed('d', modifiers);  break;
            case SDLK_e:    locker.GetViewport().KeyPressed('e', modifiers);  break;
            case SDLK_f:    window_.ToggleMaximize();                         break;
            case SDLK_g:    locker.GetViewport().KeyPressed('g', modifiers);  break;
            case SDLK_h:    locker.GetViewport().KeyPressed('h', modifiers);  break;
            case SDLK_i:    locker.GetViewport().KeyPressed('i', modifiers);  break;
            case SDLK_j:    locker.GetViewport().KeyPressed('j', modifiers);  break;
            case SDLK_k:    locker.GetViewport().KeyPressed('k', modifiers);  break;
            case SDLK_l:    locker.GetViewport().KeyPressed('l', modifiers);  break;
            case SDLK_m:    locker.GetViewport().KeyPressed('m', modifiers);  break;
            case SDLK_n:    locker.GetViewport().KeyPressed('n', modifiers);  break;
            case SDLK_o:    locker.GetViewport().KeyPressed('o', modifiers);  break;
            case SDLK_p:    locker.GetViewport().KeyPressed('p', modifiers);  break;
            case SDLK_q:    stop = true;                                      break;
            case SDLK_r:    locker.GetViewport().KeyPressed('r', modifiers);  break;
            case SDLK_s:    locker.GetViewport().KeyPressed('s', modifiers);  break;
            case SDLK_t:    locker.GetViewport().KeyPressed('t', modifiers);  break;
            case SDLK_u:    locker.GetViewport().KeyPressed('u', modifiers);  break;
            case SDLK_v:    locker.GetViewport().KeyPressed('v', modifiers);  break;
            case SDLK_w:    locker.GetViewport().KeyPressed('w', modifiers);  break;
            case SDLK_x:    locker.GetViewport().KeyPressed('x', modifiers);  break;
            case SDLK_y:    locker.GetViewport().KeyPressed('y', modifiers);  break;
            case SDLK_z:    locker.GetViewport().KeyPressed('z', modifiers);  break;
            case SDLK_KP_0: locker.GetViewport().KeyPressed('0', modifiers);  break;
            case SDLK_KP_1: locker.GetViewport().KeyPressed('1', modifiers);  break;
            case SDLK_KP_2: locker.GetViewport().KeyPressed('2', modifiers);  break;
            case SDLK_KP_3: locker.GetViewport().KeyPressed('3', modifiers);  break;
            case SDLK_KP_4: locker.GetViewport().KeyPressed('4', modifiers);  break;
            case SDLK_KP_5: locker.GetViewport().KeyPressed('5', modifiers);  break;
            case SDLK_KP_6: locker.GetViewport().KeyPressed('6', modifiers);  break;
            case SDLK_KP_7: locker.GetViewport().KeyPressed('7', modifiers);  break;
            case SDLK_KP_8: locker.GetViewport().KeyPressed('8', modifiers);  break;
            case SDLK_KP_9: locker.GetViewport().KeyPressed('9', modifiers);  break;

            case SDLK_PLUS:
            case SDLK_KP_PLUS:
              locker.GetViewport().KeyPressed('+', modifiers);  break;

            case SDLK_MINUS:
            case SDLK_KP_MINUS:
              locker.GetViewport().KeyPressed('-', modifiers);  break;

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
