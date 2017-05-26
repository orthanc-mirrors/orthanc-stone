/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "../../Resources/Orthanc/Core/Logging.h"

#include <SDL.h>

namespace OrthancStone
{
  void SdlEngine::RenderFrame()
  {
    if (!viewportChanged_)
    {
      return;
    }

    viewportChanged_ = false;

    if (buffering_.RenderOffscreen(viewport_))
    {
      // Do not notify twice when a new frame was rendered, to avoid
      // spoiling the SDL event queue
      SDL_Event event;
      SDL_memset(&event, 0, sizeof(event));
      event.type = refreshEvent_;
      event.user.code = 0;
      event.user.data1 = 0;
      event.user.data2 = 0;
      SDL_PushEvent(&event);
    }
  }


  void SdlEngine::RenderThread(SdlEngine* that)
  {
    for (;;)
    {
      that->renderFrame_.Wait();

      if (that->continue_)
      {
        that->RenderFrame();
      }
      else
      {
        return;
      }
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


  void SdlEngine::SetSize(unsigned int width,
                          unsigned int height)
  {
    buffering_.SetSize(width, height, viewport_);
    viewportChanged_ = true;
    Refresh();
  }


  void SdlEngine::Stop()
  {
    if (continue_)
    {
      continue_ = false;
      renderFrame_.Signal();  // Unlock the render thread
      renderThread_.join();
    }
  }


  void SdlEngine::Refresh()
  {
    renderFrame_.Signal();
  }


  SdlEngine::SdlEngine(SdlWindow& window,
                       IViewport& viewport) :
    window_(window),
    viewport_(viewport),
    continue_(true)
  {
    refreshEvent_ = SDL_RegisterEvents(1);

    SetSize(window_.GetWidth(), window_.GetHeight());

    viewport_.Register(*this);

    renderThread_ = boost::thread(RenderThread, this);
  }
  

  SdlEngine::~SdlEngine()
  {
    Stop();

    viewport_.Unregister(*this);
  }


  void SdlEngine::Run()
  {
    int scancodeCount = 0;
    const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

    bool stop = false;
    while (!stop)
    {
      Refresh();

      SDL_Event event;

      while (SDL_PollEvent(&event))
      {
        if (event.type == SDL_QUIT) 
        {
          stop = true;
          break;
        }
        else if (event.type == refreshEvent_)
        {
          buffering_.SwapToScreen(window_);
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
          KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          switch (event.button.button)
          {
            case SDL_BUTTON_LEFT:
              viewport_.MouseDown(MouseButton_Left, event.button.x, event.button.y, modifiers);
              break;
            
            case SDL_BUTTON_RIGHT:
              viewport_.MouseDown(MouseButton_Right, event.button.x, event.button.y, modifiers);
              break;
            
            case SDL_BUTTON_MIDDLE:
              viewport_.MouseDown(MouseButton_Middle, event.button.x, event.button.y, modifiers);
              break;

            default:
              break;
          }
        }
        else if (event.type == SDL_MOUSEMOTION)
        {
          viewport_.MouseMove(event.button.x, event.button.y);
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
          viewport_.MouseUp();
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
          switch (event.window.event)
          {
            case SDL_WINDOWEVENT_LEAVE:
              viewport_.MouseLeave();
              break;

            case SDL_WINDOWEVENT_ENTER:
              viewport_.MouseEnter();
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
            viewport_.MouseWheel(MouseWheelDirection_Up, x, y, modifiers);
          }
          else if (event.wheel.y < 0)
          {
            viewport_.MouseWheel(MouseWheelDirection_Down, x, y, modifiers);
          }
        }
        else if (event.type == SDL_KEYDOWN)
        {
          KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          switch (event.key.keysym.sym)
          {
            case SDLK_a:  viewport_.KeyPressed('a', modifiers);  break;
            case SDLK_b:  viewport_.KeyPressed('b', modifiers);  break;
            case SDLK_c:  viewport_.KeyPressed('c', modifiers);  break;
            case SDLK_d:  viewport_.KeyPressed('d', modifiers);  break;
            case SDLK_e:  viewport_.KeyPressed('e', modifiers);  break;
            case SDLK_f:  window_.ToggleMaximize();              break;
            case SDLK_g:  viewport_.KeyPressed('g', modifiers);  break;
            case SDLK_h:  viewport_.KeyPressed('h', modifiers);  break;
            case SDLK_i:  viewport_.KeyPressed('i', modifiers);  break;
            case SDLK_j:  viewport_.KeyPressed('j', modifiers);  break;
            case SDLK_k:  viewport_.KeyPressed('k', modifiers);  break;
            case SDLK_l:  viewport_.KeyPressed('l', modifiers);  break;
            case SDLK_m:  viewport_.KeyPressed('m', modifiers);  break;
            case SDLK_n:  viewport_.KeyPressed('n', modifiers);  break;
            case SDLK_o:  viewport_.KeyPressed('o', modifiers);  break;
            case SDLK_p:  viewport_.KeyPressed('p', modifiers);  break;
            case SDLK_q:  stop = true;                           break;
            case SDLK_r:  viewport_.KeyPressed('r', modifiers);  break;
            case SDLK_s:  viewport_.KeyPressed('s', modifiers);  break;
            case SDLK_t:  viewport_.KeyPressed('t', modifiers);  break;
            case SDLK_u:  viewport_.KeyPressed('u', modifiers);  break;
            case SDLK_v:  viewport_.KeyPressed('v', modifiers);  break;
            case SDLK_w:  viewport_.KeyPressed('w', modifiers);  break;
            case SDLK_x:  viewport_.KeyPressed('x', modifiers);  break;
            case SDLK_y:  viewport_.KeyPressed('y', modifiers);  break;
            case SDLK_z:  viewport_.KeyPressed('z', modifiers);  break;

            default:
              break;
          }
        }
      }

      SDL_Delay(10);   // Necessary for mouse wheel events to work
    }

    Stop();
  }


  void SdlEngine::GlobalInitialize()
  {
    SDL_Init(SDL_INIT_VIDEO);
  }


  void SdlEngine::GlobalFinalize()
  {
    SDL_Quit();
  }
}

#endif
