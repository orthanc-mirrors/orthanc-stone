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

#include "../../../Resources/Orthanc/Core/Logging.h"

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

    bool updated;
    
    {
      BasicApplicationContext::ViewportLocker locker(context_);
      updated = buffering_.RenderOffscreen(locker.GetViewport());
    }

    if (updated)
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


  void SdlEngine::SetSize(BasicApplicationContext::ViewportLocker& locker,
                          unsigned int width,
                          unsigned int height)
  {
    buffering_.SetSize(width, height, locker.GetViewport());
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
                       BasicApplicationContext& context) :
    window_(window),
    context_(context),
    continue_(true)
  {
    refreshEvent_ = SDL_RegisterEvents(1);

    {
      BasicApplicationContext::ViewportLocker locker(context_);
      SetSize(locker, window_.GetWidth(), window_.GetHeight());
      locker.GetViewport().Register(*this);
    }

    renderThread_ = boost::thread(RenderThread, this);
  }
  

  SdlEngine::~SdlEngine()
  {
    Stop();

    {
      BasicApplicationContext::ViewportLocker locker(context_);
      locker.GetViewport().Unregister(*this);
    }
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
        BasicApplicationContext::ViewportLocker locker(context_);

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
        else if (event.type == SDL_KEYDOWN)
        {
          KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          switch (event.key.keysym.sym)
          {
            case SDLK_a:  locker.GetViewport().KeyPressed('a', modifiers);  break;
            case SDLK_b:  locker.GetViewport().KeyPressed('b', modifiers);  break;
            case SDLK_c:  locker.GetViewport().KeyPressed('c', modifiers);  break;
            case SDLK_d:  locker.GetViewport().KeyPressed('d', modifiers);  break;
            case SDLK_e:  locker.GetViewport().KeyPressed('e', modifiers);  break;
            case SDLK_f:  window_.ToggleMaximize();                         break;
            case SDLK_g:  locker.GetViewport().KeyPressed('g', modifiers);  break;
            case SDLK_h:  locker.GetViewport().KeyPressed('h', modifiers);  break;
            case SDLK_i:  locker.GetViewport().KeyPressed('i', modifiers);  break;
            case SDLK_j:  locker.GetViewport().KeyPressed('j', modifiers);  break;
            case SDLK_k:  locker.GetViewport().KeyPressed('k', modifiers);  break;
            case SDLK_l:  locker.GetViewport().KeyPressed('l', modifiers);  break;
            case SDLK_m:  locker.GetViewport().KeyPressed('m', modifiers);  break;
            case SDLK_n:  locker.GetViewport().KeyPressed('n', modifiers);  break;
            case SDLK_o:  locker.GetViewport().KeyPressed('o', modifiers);  break;
            case SDLK_p:  locker.GetViewport().KeyPressed('p', modifiers);  break;
            case SDLK_q:  stop = true;                                      break;
            case SDLK_r:  locker.GetViewport().KeyPressed('r', modifiers);  break;
            case SDLK_s:  locker.GetViewport().KeyPressed('s', modifiers);  break;
            case SDLK_t:  locker.GetViewport().KeyPressed('t', modifiers);  break;
            case SDLK_u:  locker.GetViewport().KeyPressed('u', modifiers);  break;
            case SDLK_v:  locker.GetViewport().KeyPressed('v', modifiers);  break;
            case SDLK_w:  locker.GetViewport().KeyPressed('w', modifiers);  break;
            case SDLK_x:  locker.GetViewport().KeyPressed('x', modifiers);  break;
            case SDLK_y:  locker.GetViewport().KeyPressed('y', modifiers);  break;
            case SDLK_z:  locker.GetViewport().KeyPressed('z', modifiers);  break;

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
