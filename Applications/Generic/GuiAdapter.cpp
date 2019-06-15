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

#include "GuiAdapter.h"

#if ORTHANC_ENABLE_SDL == 1
# if ORTHANC_ENABLE_OPENGL == 1
#   include "../../Framework/OpenGL/OpenGLIncludes.h"
#   include <SDL_video.h>
#   include <SDL_render.h>
#   include <SDL.h>
# endif
#endif

#if ORTHANC_ENABLE_THREADS == 1
# include "../../Framework/Messages/LockingEmitter.h"
#endif

namespace OrthancStone
{
  void GuiAdapter::RegisterWidget(boost::shared_ptr<IGuiAdapterWidget> widget)
  {
    widgets_.push_back(widget);
  }

#if ORTHANC_ENABLE_WASM == 1
  void GuiAdapter::Run()
  {
  }

  void ConvertFromPlatform(
    GuiAdapterUiEvent& dest,
    int                      eventType,
    const EmscriptenUiEvent& src)
  {
    // no data for now
  }

  void ConvertFromPlatform(
    GuiAdapterMouseEvent& dest,
    int                         eventType,
    const EmscriptenMouseEvent& src)
  {
    memset(&dest, 0, sizeof(GuiAdapterMouseEvent));
    switch (eventType)
    {
    case EMSCRIPTEN_EVENT_CLICK:
      LOG(ERROR) << "Emscripten EMSCRIPTEN_EVENT_CLICK is not supported";
      ORTHANC_ASSERT(false, "Not supported");
      break;
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
      dest.type = GUIADAPTER_EVENT_MOUSEDOWN;
      break;
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      dest.type = GUIADAPTER_EVENT_MOUSEMOVE;
      break;
    case EMSCRIPTEN_EVENT_MOUSEUP:
      dest.type = GUIADAPTER_EVENT_MOUSEUP;
      break;
    case EMSCRIPTEN_EVENT_WHEEL:
      dest.type = GUIADAPTER_EVENT_WHEEL;
      break;

    default:
      LOG(ERROR) << "Emscripten event: " << eventType << " is not supported";
      ORTHANC_ASSERT(false, "Not supported");
    }
    //dest.timestamp = src.timestamp;
    //dest.screenX = src.screenX;
    //dest.screenY = src.screenY;
    //dest.clientX = src.clientX;
    //dest.clientY = src.clientY;
    dest.ctrlKey = src.ctrlKey;
    dest.shiftKey = src.shiftKey;
    dest.altKey = src.altKey;
    //dest.metaKey = src.metaKey;
    dest.button = src.button;
    //dest.buttons = src.buttons;
    //dest.movementX = src.movementX;
    //dest.movementY = src.movementY;
    dest.targetX = src.targetX;
    dest.targetY = src.targetY;
    //dest.canvasX = src.canvasX;
    //dest.canvasY = src.canvasY;
    //dest.padding = src.padding;
  }

  void ConvertFromPlatform(
    GuiAdapterWheelEvent& dest,
    int                         eventType,
    const EmscriptenWheelEvent& src)
  {
    ConvertFromPlatform(dest.mouse, eventType, src.mouse);
    dest.deltaX = src.deltaX;
    dest.deltaY = src.deltaY;
    switch (src.deltaMode)
    {
    case DOM_DELTA_PIXEL:
      dest.deltaMode = GUIADAPTER_DELTA_PIXEL;
      break;
    case DOM_DELTA_LINE:
      dest.deltaMode = GUIADAPTER_DELTA_LINE;
      break;
    case DOM_DELTA_PAGE:
      dest.deltaMode = GUIADAPTER_DELTA_PAGE;
      break;
    default:
      ORTHANC_ASSERT(false, "Unknown deltaMode: " << src.deltaMode <<
        " in wheel event...");
    }
    dest.deltaMode = src.deltaMode;
  }

  void ConvertFromPlatform(
    GuiAdapterKeyboardEvent& dest,
    const EmscriptenKeyboardEvent& src)
  {
    dest.ctrlKey = src.ctrlKey;
    dest.shiftKey = src.shiftKey;
    dest.altKey = src.altKey;
  }
   
  template<typename GenericFunc>
  struct FuncAdapterPayload
  {
    void*       userData;
    GenericFunc callback;
  };

  template<typename GenericFunc,
           typename GuiAdapterEvent,
           typename EmscriptenEvent>
  EM_BOOL OnEventAdapterFunc(
    int eventType, const EmscriptenEvent* wheelEvent, void* userData)
  {

    // userData is OnMouseWheelFuncAdapterPayload
    FuncAdapterPayload<GenericFunc>* payload =
      reinterpret_cast<FuncAdapterPayload<GenericFunc>*>(userData);
    // LOG(INFO) << "OnEventAdapterFunc";
    // LOG(INFO) << "------------------";
    // LOG(INFO) << "eventType: " << eventType << " wheelEvent: " << 
    //   (int)wheelEvent << " userData: " << userData << 
    //   " payload->userData: " << payload->userData;
    
    GuiAdapterEvent guiEvent;
    ConvertFromPlatform(guiEvent, eventType, *wheelEvent);
    bool ret = (*(payload->callback))(&guiEvent, payload->userData);
    return static_cast<EM_BOOL>(ret);
  }

  template<typename GenericFunc,
    typename GuiAdapterEvent,
    typename EmscriptenEvent>
    EM_BOOL OnEventAdapterFunc2(
      int /*eventType*/, const EmscriptenEvent* wheelEvent, void* userData)
  {
    // userData is OnMouseWheelFuncAdapterPayload
    FuncAdapterPayload<GenericFunc>* payload =
      reinterpret_cast<FuncAdapterPayload<GenericFunc>*>(userData);
    
    GuiAdapterEvent guiEvent;
    ConvertFromPlatform(guiEvent, *wheelEvent);
    bool ret = (*(payload->callback))(&guiEvent, payload->userData);
    return static_cast<EM_BOOL>(ret);
  }

  template<typename GenericFunc>
    EM_BOOL OnEventAdapterFunc3(
      double time, void* userData)
  {
    // userData is OnMouseWheelFuncAdapterPayload
    FuncAdapterPayload<GenericFunc>* payload =
      reinterpret_cast<FuncAdapterPayload<GenericFunc>*>(userData);
    //std::auto_ptr< FuncAdapterPayload<GenericFunc> > deleter(payload);
    bool ret = (*(payload->callback))(time, payload->userData);
    return static_cast<EM_BOOL>(ret);
  }

  // resize: (const char* target, void* userData, EM_BOOL useCapture, em_ui_callback_func callback)
  template<
    typename GenericFunc, 
    typename GuiAdapterEvent, 
    typename EmscriptenEvent, 
    typename EmscriptenSetCallbackFunc>
  static void SetCallback(
    EmscriptenSetCallbackFunc emFunc, 
    std::string canvasId, void* userData, bool capture, GenericFunc func)
  {
    // TODO: write RemoveCallback with an int id that gets returned from
    // here
    FuncAdapterPayload<GenericFunc>* payload = 
      new FuncAdapterPayload<GenericFunc>();
    std::auto_ptr<FuncAdapterPayload<GenericFunc> > payloadP(payload);
    payload->callback = func;
    payload->userData = userData;
    void* userDataRaw = reinterpret_cast<void*>(payload);
    // LOG(INFO) << "SetCallback -- userDataRaw: " << userDataRaw <<
    //   " payload: " << payload << " payload->userData: " << payload->userData;
    (*emFunc)(
      canvasId.c_str(),
      userDataRaw,
      static_cast<EM_BOOL>(capture),
      &OnEventAdapterFunc<GenericFunc, GuiAdapterEvent, EmscriptenEvent>,
      EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD);
    payloadP.release();
  }

  template<
    typename GenericFunc,
    typename GuiAdapterEvent,
    typename EmscriptenEvent,
    typename EmscriptenSetCallbackFunc>
    static void SetCallback2(
      EmscriptenSetCallbackFunc emFunc,
      std::string canvasId, void* userData, bool capture, GenericFunc func)
  {
    std::auto_ptr<FuncAdapterPayload<GenericFunc> > payload(
      new FuncAdapterPayload<GenericFunc>()
    );
    payload->callback = func;
    payload->userData = userData;
    void* userDataRaw = reinterpret_cast<void*>(payload.release());
    (*emFunc)(
      canvasId.c_str(),
      userDataRaw,
      static_cast<EM_BOOL>(capture),
      &OnEventAdapterFunc2<GenericFunc, GuiAdapterEvent, EmscriptenEvent>,
      EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD);
  }

  template<
    typename GenericFunc,
    typename EmscriptenSetCallbackFunc>
    static void SetCallback3(
      EmscriptenSetCallbackFunc emFunc,
      void* userData, GenericFunc func)
  {
    // LOG(ERROR) << "SetCallback3 !!!!!! (RequestAnimationFrame)";
    std::auto_ptr<FuncAdapterPayload<GenericFunc> > payload(
      new FuncAdapterPayload<GenericFunc>()
    );
    payload->callback = func;
    payload->userData = userData;
    void* userDataRaw = reinterpret_cast<void*>(payload.release());
    (*emFunc)(
      &OnEventAdapterFunc3<GenericFunc>,
      userDataRaw);
  }

  void GuiAdapter::SetWheelCallback(
    std::string canvasId, void* userData, bool capture, OnMouseWheelFunc func)
  {
    SetCallback<OnMouseWheelFunc, GuiAdapterWheelEvent, EmscriptenWheelEvent>(
      &emscripten_set_wheel_callback_on_thread,
      canvasId,
      userData,
      capture,
      func);
  }

  void GuiAdapter::SetMouseDownCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc func)
  {
    SetCallback<OnMouseEventFunc, GuiAdapterMouseEvent, EmscriptenMouseEvent>(
      &emscripten_set_mousedown_callback_on_thread,
      canvasId,
      userData,
      capture,
      func);
  }

  void GuiAdapter::SetMouseMoveCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc func)
  {
    // LOG(INFO) << "SetMouseMoveCallback -- " << "supplied userData: " << 
    //   userData;

    SetCallback<OnMouseEventFunc, GuiAdapterMouseEvent, EmscriptenMouseEvent>(
      &emscripten_set_mousemove_callback_on_thread,
      canvasId,
      userData,
      capture,
      func);
  }

  void GuiAdapter::SetMouseUpCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc func)
  {
    SetCallback<OnMouseEventFunc, GuiAdapterMouseEvent, EmscriptenMouseEvent>(
      &emscripten_set_mouseup_callback_on_thread,
      canvasId,
      userData,
      capture,
      func);
  }

  void GuiAdapter::SetKeyDownCallback(
    std::string canvasId, void* userData, bool capture, OnKeyDownFunc func)
  {
    SetCallback2<OnKeyDownFunc, GuiAdapterKeyboardEvent, EmscriptenKeyboardEvent>(
      &emscripten_set_keydown_callback_on_thread,
      canvasId,
      userData,
      capture,
      func);
  }

  void GuiAdapter::SetKeyUpCallback(
    std::string canvasId, void* userData, bool capture, OnKeyUpFunc func)
  {
    SetCallback2<OnKeyUpFunc, GuiAdapterKeyboardEvent, EmscriptenKeyboardEvent>(
      &emscripten_set_keyup_callback_on_thread,
      canvasId,
      userData,
      capture,
      func);
  }

  void GuiAdapter::SetResizeCallback(
    std::string canvasId, void* userData, bool capture, OnWindowResizeFunc func)
  {
    SetCallback<OnWindowResizeFunc, GuiAdapterUiEvent, EmscriptenUiEvent>(
      &emscripten_set_resize_callback_on_thread,
      canvasId,
      userData,
      capture,
      func);
  }

  void GuiAdapter::RequestAnimationFrame(
    OnAnimationFrameFunc func, void* userData)
  {
    // LOG(ERROR) << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+";
    // LOG(ERROR) << "RequestAnimationFrame";
    // LOG(ERROR) << "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+";
    SetCallback3<OnAnimationFrameFunc>(
      &emscripten_request_animation_frame_loop,
      userData,
      func);
  }

#if 0
  void GuiAdapter::SetKeyDownCallback(
    std::string canvasId, void* userData, bool capture, OnKeyDownFunc func)
  {
    emscripten_set_keydown_callback(canvasId.c_str(), userData, static_cast<EM_BOOL>(capture), func);
  }
  void GuiAdapter::SetKeyUpCallback(
    std::string canvasId, void* userData, bool capture, OnKeyUpFunc func)
  {
    emscripten_set_keyup_callback(canvasId.c_str(), userData, static_cast<EM_BOOL>(capture), func);
  }

  void GuiAdapter::SetResizeCallback(std::string canvasId, void* userData, bool capture, OnWindowResizeFunc func)
  {
    emscripten_set_resize_callback(canvasId.c_str(), userData, static_cast<EM_BOOL>(capture), func);
  }

  void GuiAdapter::RequestAnimationFrame(OnAnimationFrameFunc func, void* userData)
  {
    emscripten_request_animation_frame_loop(func, userData);
  }
#endif


#else

void ConvertFromPlatform(
  GuiAdapterMouseEvent& dest,
  bool ctrlPressed, bool shiftPressed, bool altPressed,
  const SDL_Event& source)
{
  memset(&dest, 0, sizeof(GuiAdapterMouseEvent));
  switch (source.type)
  {
  case SDL_MOUSEBUTTONDOWN:
    dest.type = GUIADAPTER_EVENT_MOUSEDOWN;
    break;
  case SDL_MOUSEMOTION:
    dest.type = GUIADAPTER_EVENT_MOUSEMOVE;
    break;
  case SDL_MOUSEBUTTONUP:
    dest.type = GUIADAPTER_EVENT_MOUSEUP;
    break;
  default:
    LOG(ERROR) << "SDL event: " << source.type << " is not supported";
    ORTHANC_ASSERT(false, "Not supported");
    }
  //dest.timestamp = src.timestamp;
  //dest.screenX = src.screenX;
  //dest.screenY = src.screenY;
  //dest.clientX = src.clientX;
  //dest.clientY = src.clientY;
  dest.ctrlKey = ctrlPressed;
  dest.shiftKey = shiftPressed;
  dest.altKey = altPressed;
  //dest.metaKey = src.metaKey;
  switch (source.button.button)
  {
  case SDL_BUTTON_MIDDLE:
    dest.button = 1;
    break;

  case SDL_BUTTON_RIGHT:
    dest.button = 2;
    break;

  case SDL_BUTTON_LEFT:
    dest.button = 0;
    break;

  default:
    break;
  }
  //dest.buttons = src.buttons;
  //dest.movementX = src.movementX;
  //dest.movementY = src.movementY;
  dest.targetX = source.button.x;
  dest.targetY = source.button.y;
  //dest.canvasX = src.canvasX;
  //dest.canvasY = src.canvasY;
  //dest.padding = src.padding;
  }

  void GuiAdapter::SetResizeCallback(
    std::string canvasId, void* userData, bool capture, OnWindowResizeFunc func)
  {
    resizeHandlers_.push_back(std::make_pair(func, userData));
  }

  void GuiAdapter::SetMouseDownCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc func)
 {
 }

  void GuiAdapter::SetMouseMoveCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc  func)
 {
 }

  void GuiAdapter::SetMouseUpCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc  func)
 {
 }

 void GuiAdapter::SetWheelCallback(
   std::string canvasId, void* userData, bool capture, OnMouseWheelFunc  func)
 {
 }

 void GuiAdapter::SetKeyDownCallback(
   std::string canvasId, void* userData, bool capture, OnKeyDownFunc   func)
 {
 }

 void GuiAdapter::SetKeyUpCallback(
   std::string canvasId, void* userData, bool capture, OnKeyUpFunc    func)
 {
 }


  void GuiAdapter::OnAnimationFrame()
  {
    for (size_t i = 0; i < animationFrameHandlers_.size(); i++)
    {
      // TODO: fix time 
      (*(animationFrameHandlers_[i].first))(0, animationFrameHandlers_[i].second);
    }
  }

  void GuiAdapter::OnResize()
  {
    for (size_t i = 0; i < resizeHandlers_.size(); i++)
    {
      // TODO: fix time 
      (*(resizeHandlers_[i].first))(0, resizeHandlers_[i].second);
    }
  }
   
  void GuiAdapter::OnMouseEvent(uint32_t windowID, const GuiAdapterMouseEvent& event)
  {
  }


  void GuiAdapter::RequestAnimationFrame(OnAnimationFrameFunc func, void* userData)
  {
    animationFrameHandlers_.push_back(std::make_pair(func, userData));
  }

# if ORTHANC_ENABLE_OPENGL == 1

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
# endif

  void GuiAdapter::Run()
  {
# if ORTHANC_ENABLE_OPENGL == 1
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(OpenGLMessageCallback, 0);
# endif

    // Uint32 SDL_GetWindowID(SDL_Window* window)
    // SDL_Window* SDL_GetWindowFromID(Uint32 id)   // may return NULL

    bool stop = false;
    while (!stop)
    {
      {
        LockingEmitter::WriterLock lock(lockingEmitter_);
        OnAnimationFrame(); // in SDL we must call it
      }

      SDL_Event event;

      while (!stop && SDL_PollEvent(&event))
      {
        LockingEmitter::WriterLock lock(lockingEmitter_);

        if (event.type == SDL_QUIT)
        {
          // TODO: call exit callbacks here
          stop = true;
          break;
        }
        else if ( (event.type == SDL_MOUSEMOTION) ||
                  (event.type == SDL_MOUSEBUTTONDOWN) ||
                  (event.type == SDL_MOUSEBUTTONUP) )
        {
          int scancodeCount = 0;
          const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);
          bool ctrlPressed(false);
          bool shiftPressed(false);
          bool altPressed(false);
            
          if (SDL_SCANCODE_LCTRL < scancodeCount && keyboardState[SDL_SCANCODE_LCTRL])
            ctrlPressed = true;
          if (SDL_SCANCODE_RCTRL < scancodeCount && keyboardState[SDL_SCANCODE_RCTRL])
            ctrlPressed = true;
          if (SDL_SCANCODE_LSHIFT < scancodeCount && keyboardState[SDL_SCANCODE_LSHIFT])
            shiftPressed = true;
          if (SDL_SCANCODE_RSHIFT < scancodeCount && keyboardState[SDL_SCANCODE_RSHIFT])
            shiftPressed = true;
          if (SDL_SCANCODE_LALT < scancodeCount && keyboardState[SDL_SCANCODE_LALT])
            altPressed = true;
          
          GuiAdapterMouseEvent dest;
          ConvertFromPlatform(dest, ctrlPressed, shiftPressed, altPressed, event);
          OnMouseEvent(event.window.windowID, dest);
#if 0
          // for reference, how to create trackers
          if (tracker)
          {
            PointerEvent e;
            e.AddPosition(compositor.GetPixelCenterCoordinates(
              event.button.x, event.button.y));
            tracker->PointerMove(e);
          }
#endif
        }
        else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
#if 0
          tracker.reset();
#endif
          OnResize();
        }
        else if (event.type == SDL_KEYDOWN && event.key.repeat == 0 /* Ignore key bounce */)
        {
          switch (event.key.keysym.sym)
          {
          case SDLK_f:
            // window.GetWindow().ToggleMaximize(); //TODO: move to particular handler
            break;

          case SDLK_s:
#if 0
            // TODO: re-enable at application-level!!!!
            VisitWidgets(
              [](auto value)
              {
                auto widget = boost::dynamic_pointer_cast<VolumeSlicerWidget()
                value->FitContent();
              });
#endif
            break;

          case SDLK_q:
            stop = true;
            break;

          default:
            break;
          }
        }
        //        HandleApplicationEvent(controller, compositor, event, tracker);
      }

      SDL_Delay(1);
    }
  }
#endif
}

