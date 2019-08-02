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

#if ORTHANC_ENABLE_OPENGL == 1
#  include "../../Framework/OpenGL/OpenGLIncludes.h"
#endif

#if ORTHANC_ENABLE_SDL == 1
#  include <SDL_video.h>
#  include <SDL_render.h>
#  include <SDL.h>
#endif

#if ORTHANC_ENABLE_THREADS == 1
#  include "../../Framework/Messages/LockingEmitter.h"
#endif

namespace OrthancStone
{
  void GuiAdapter::RegisterWidget(boost::shared_ptr<IGuiAdapterWidget> widget)
  {
    widgets_.push_back(widget);
  }

  std::ostream& operator<<(
    std::ostream& os, const GuiAdapterKeyboardEvent& event)
  {
    os << "sym: " << event.sym << " (" << (int)(event.sym[0]) << ") ctrl: " << event.ctrlKey << ", " <<
      "shift: " << event.shiftKey << ", " <<
      "alt: " << event.altKey;
    return os;
  }

#if ORTHANC_ENABLE_WASM == 1
  void GuiAdapter::Run(GuiAdapterRunFunc /*func*/, void* /*cookie*/)
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

  void ConvertFromPlatform( GuiAdapterWheelEvent& dest, int eventType, const EmscriptenWheelEvent& src)
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

  void ConvertFromPlatform(GuiAdapterKeyboardEvent& dest, const EmscriptenKeyboardEvent& src)
  {
    dest.sym[0] = src.key[0];
    dest.sym[1] = 0;
    dest.ctrlKey = src.ctrlKey;
    dest.shiftKey = src.shiftKey;
    dest.altKey = src.altKey;
  }

  template<typename GenericFunc>
  struct FuncAdapterPayload
  {
    std::string canvasId;
    void* userData;
    GenericFunc callback;
  };

  template<typename GenericFunc,
    typename GuiAdapterEvent,
    typename EmscriptenEvent>
    EM_BOOL OnEventAdapterFunc(
      int eventType, const EmscriptenEvent* emEvent, void* userData)
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
    ConvertFromPlatform(guiEvent, eventType, *emEvent);
    bool ret = (*(payload->callback))(payload->canvasId, &guiEvent, payload->userData);
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
    bool ret = (*(payload->callback))(payload->canvasId, &guiEvent, payload->userData);
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
    payload->canvasId = canvasId;
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
    payload->canvasId = canvasId;
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
    static void SetAnimationFrameCallback(
      EmscriptenSetCallbackFunc emFunc,
      void* userData, GenericFunc func)
  {
    // LOG(ERROR) << "SetAnimationFrameCallback !!!!!! (RequestAnimationFrame)";
    std::auto_ptr<FuncAdapterPayload<GenericFunc> > payload(
      new FuncAdapterPayload<GenericFunc>()
    );
    payload->canvasId = "UNDEFINED";
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
    SetAnimationFrameCallback<OnAnimationFrameFunc>(
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

  // SDL ONLY
  void ConvertFromPlatform(GuiAdapterMouseEvent& dest, bool ctrlPressed, bool shiftPressed, bool altPressed, const SDL_Event& source)
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
    case SDL_MOUSEWHEEL:
      dest.type = GUIADAPTER_EVENT_WHEEL;
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
      dest.button =GUIADAPTER_MOUSEBUTTON_MIDDLE;
      break;

    case SDL_BUTTON_RIGHT:
      dest.button = GUIADAPTER_MOUSEBUTTON_RIGHT;
      break;

    case SDL_BUTTON_LEFT:
      dest.button = GUIADAPTER_MOUSEBUTTON_LEFT;
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

  void ConvertFromPlatform(
    GuiAdapterWheelEvent& dest,
    bool ctrlPressed, bool shiftPressed, bool altPressed,
    const SDL_Event& source)
  {
    ConvertFromPlatform(dest.mouse, ctrlPressed, shiftPressed, altPressed, source);
    dest.deltaX = source.wheel.x;
    dest.deltaY = source.wheel.y;
  }

  void ConvertFromPlatform(GuiAdapterKeyboardEvent& dest, const SDL_Event& src)
  {
    memset(&dest, 0, sizeof(GuiAdapterMouseEvent));
    switch (src.type)
    {
    case SDL_KEYDOWN:
      dest.type = GUIADAPTER_EVENT_KEYDOWN;
      break;
    case SDL_KEYUP:
      dest.type = GUIADAPTER_EVENT_KEYUP;
      break;
    default:
      LOG(ERROR) << "SDL event: " << src.type << " is not supported";
      ORTHANC_ASSERT(false, "Not supported");
    }
    dest.sym[0] = src.key.keysym.sym;
    dest.sym[1] = 0;

    if (src.key.keysym.mod & KMOD_CTRL)
      dest.ctrlKey = true;
    else
      dest.ctrlKey = false;

    if (src.key.keysym.mod & KMOD_SHIFT)
      dest.shiftKey = true;
    else
      dest.shiftKey = false;

    if (src.key.keysym.mod & KMOD_ALT)
      dest.altKey = true;
    else
      dest.altKey = false;
  }



  // SDL ONLY
  void GuiAdapter::SetResizeCallback(
    std::string canvasId, void* userData, bool capture, OnWindowResizeFunc func)
  {
    resizeHandlers_.push_back(EventHandlerData<OnWindowResizeFunc>(canvasId, func, userData));
  }

  // SDL ONLY
  void GuiAdapter::SetMouseDownCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc func)
  {
    mouseDownHandlers_.push_back(EventHandlerData<OnMouseEventFunc>(canvasId, func, userData));
  }

  // SDL ONLY
  void GuiAdapter::SetMouseMoveCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc  func)
  {
    mouseMoveHandlers_.push_back(EventHandlerData<OnMouseEventFunc>(canvasId, func, userData));
  }

  // SDL ONLY
  void GuiAdapter::SetMouseUpCallback(
    std::string canvasId, void* userData, bool capture, OnMouseEventFunc  func)
  {
    mouseUpHandlers_.push_back(EventHandlerData<OnMouseEventFunc>(canvasId, func, userData));
  }

  // SDL ONLY
  void GuiAdapter::SetWheelCallback(
    std::string canvasId, void* userData, bool capture, OnMouseWheelFunc  func)
  {
    mouseWheelHandlers_.push_back(EventHandlerData<OnMouseWheelFunc>(canvasId, func, userData));
  }

  // SDL ONLY
  void GuiAdapter::SetKeyDownCallback(
    std::string canvasId, void* userData, bool capture, OnKeyDownFunc   func)
  {
    keyDownHandlers_.push_back(EventHandlerData<OnKeyDownFunc>(canvasId, func, userData));
  }

  // SDL ONLY
  void GuiAdapter::SetKeyUpCallback(
    std::string canvasId, void* userData, bool capture, OnKeyUpFunc    func)
  {
    keyUpHandlers_.push_back(EventHandlerData<OnKeyUpFunc>(canvasId, func, userData));
  }

  // SDL ONLY
  void GuiAdapter::OnAnimationFrame()
  {
    for (size_t i = 0; i < animationFrameHandlers_.size(); i++)
    {
      // TODO: fix time 
      (*(animationFrameHandlers_[i].first))(0, animationFrameHandlers_[i].second);
    }
  }

  // SDL ONLY
  void GuiAdapter::OnResize()
  {
    for (size_t i = 0; i < resizeHandlers_.size(); i++)
    {
      (*(resizeHandlers_[i].func))(
        resizeHandlers_[i].canvasName, 0, resizeHandlers_[i].userData);
    }
  }

  // SDL ONLY
  void GuiAdapter::OnMouseWheelEvent(uint32_t windowID, const GuiAdapterWheelEvent& event)
  {

    // the SDL window name IS the canvas name ("canvas" is used because this lib
    // is designed for Wasm
    SDL_Window* sdlWindow = SDL_GetWindowFromID(windowID);
    ORTHANC_ASSERT(sdlWindow != NULL, "Window ID \"" << windowID << "\" is not a valid SDL window ID!");

    const char* windowTitleSz = SDL_GetWindowTitle(sdlWindow);
    ORTHANC_ASSERT(windowTitleSz != NULL, "Window ID \"" << windowID << "\" has a NULL window title!");

    std::string windowTitle(windowTitleSz);
    ORTHANC_ASSERT(windowTitle != "", "Window ID \"" << windowID << "\" has an empty window title!");

    switch (event.mouse.type)
    {
    case GUIADAPTER_EVENT_WHEEL:
      for (size_t i = 0; i < mouseWheelHandlers_.size(); i++)
      {
        if (mouseWheelHandlers_[i].canvasName == windowTitle)
          (*(mouseWheelHandlers_[i].func))(windowTitle, &event, mouseWheelHandlers_[i].userData);
      }
      break;
    default:
      ORTHANC_ASSERT(false, "Wrong event.type: " << event.mouse.type << " in GuiAdapter::OnMouseWheelEvent(...)");
      break;
    }
  }


  void GuiAdapter::OnKeyboardEvent(uint32_t windowID, const GuiAdapterKeyboardEvent& event)
  {
    // only one-letter (ascii) keyboard events supported for now
    ORTHANC_ASSERT(event.sym[0] != 0);
    ORTHANC_ASSERT(event.sym[1] == 0);

    SDL_Window* sdlWindow = SDL_GetWindowFromID(windowID);
    ORTHANC_ASSERT(sdlWindow != NULL, "Window ID \"" << windowID << "\" is not a valid SDL window ID!");

    const char* windowTitleSz = SDL_GetWindowTitle(sdlWindow);
    ORTHANC_ASSERT(windowTitleSz != NULL, "Window ID \"" << windowID << "\" has a NULL window title!");

    std::string windowTitle(windowTitleSz);
    ORTHANC_ASSERT(windowTitle != "", "Window ID \"" << windowID << "\" has an empty window title!");

    switch (event.type)
    {
    case GUIADAPTER_EVENT_KEYDOWN:
      for (size_t i = 0; i < keyDownHandlers_.size(); i++)
      {
        (*(keyDownHandlers_[i].func))(windowTitle, &event, keyDownHandlers_[i].userData);
      }
      break;
    case GUIADAPTER_EVENT_KEYUP:
      for (size_t i = 0; i < keyUpHandlers_.size(); i++)
      {
        (*(keyUpHandlers_[i].func))(windowTitle, &event, keyUpHandlers_[i].userData);
      }
      break;
    default:
      ORTHANC_ASSERT(false, "Wrong event.type: " << event.type << " in GuiAdapter::OnKeyboardEvent(...)");
      break;
    }
  }

  // SDL ONLY
  void GuiAdapter::OnMouseEvent(uint32_t windowID, const GuiAdapterMouseEvent& event)
  {
    if (windowID == 0)
    {
      LOG(WARNING) << "GuiAdapter::OnMouseEvent -- windowID == 0 and event won't be routed!";
    }
    else
    {
      // the SDL window name IS the canvas name ("canvas" is used because this lib
      // is designed for Wasm
      SDL_Window* sdlWindow = SDL_GetWindowFromID(windowID);

      ORTHANC_ASSERT(sdlWindow != NULL, "Window ID \"" << windowID << "\" is not a valid SDL window ID!");

      const char* windowTitleSz = SDL_GetWindowTitle(sdlWindow);
      ORTHANC_ASSERT(windowTitleSz != NULL, "Window ID \"" << windowID << "\" has a NULL window title!");

      std::string windowTitle(windowTitleSz);
      ORTHANC_ASSERT(windowTitle != "", "Window ID \"" << windowID << "\" has an empty window title!");

      switch (event.type)
      {
      case GUIADAPTER_EVENT_MOUSEDOWN:
        for (size_t i = 0; i < mouseDownHandlers_.size(); i++)
        {
          if (mouseDownHandlers_[i].canvasName == windowTitle)
            (*(mouseDownHandlers_[i].func))(windowTitle, &event, mouseDownHandlers_[i].userData);
        }
        break;
      case GUIADAPTER_EVENT_MOUSEMOVE:
        for (size_t i = 0; i < mouseMoveHandlers_.size(); i++)
        {
          if (mouseMoveHandlers_[i].canvasName == windowTitle)
            (*(mouseMoveHandlers_[i].func))(windowTitle, &event, mouseMoveHandlers_[i].userData);
        }
        break;
      case GUIADAPTER_EVENT_MOUSEUP:
        for (size_t i = 0; i < mouseUpHandlers_.size(); i++)
        {
          if (mouseUpHandlers_[i].canvasName == windowTitle)
            (*(mouseUpHandlers_[i].func))(windowTitle, &event, mouseUpHandlers_[i].userData);
        }
        break;
      default:
        ORTHANC_ASSERT(false, "Wrong event.type: " << event.type << " in GuiAdapter::OnMouseEvent(...)");
        break;
      }

      ////boost::shared_ptr<IGuiAdapterWidget> GetWidgetFromWindowId();
      //boost::shared_ptr<IGuiAdapterWidget> foundWidget;
      //VisitWidgets([foundWidget, windowID](auto widget)
      //  {
      //    if (widget->GetSdlWindowID() == windowID)
      //      foundWidget = widget;
      //  });
      //ORTHANC_ASSERT(foundWidget, "WindowID " << windowID << " was not found in the registered widgets!");
      //if(foundWidget)
      //  foundWidget->
    }
  }

  // SDL ONLY
  void GuiAdapter::RequestAnimationFrame(OnAnimationFrameFunc func, void* userData)
  {
    animationFrameHandlers_.push_back(std::make_pair(func, userData));
  }

# if ORTHANC_ENABLE_OPENGL == 1 && !defined(__APPLE__)   /* OpenGL debug is not available on OS X */

  // SDL ONLY
  static void GLAPIENTRY
    OpenGLMessageCallback(GLenum source,
      GLenum type,
      GLuint id,
      GLenum severity,
      GLsizei length,
      const GLchar * message,
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

  // SDL ONLY
  void GuiAdapter::Run(GuiAdapterRunFunc func, void* cookie)
  {
#if 1
    // TODO: MAKE THIS DYNAMIC !!! See SdlOpenGLViewport vs Cairo in ViewportWrapper
# if ORTHANC_ENABLE_OPENGL == 1 && !defined(__APPLE__)
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(OpenGLMessageCallback, 0);
# endif
#endif

    // Uint32 SDL_GetWindowID(SDL_Window* window)
    // SDL_Window* SDL_GetWindowFromID(Uint32 id)   // may return NULL

    bool stop = false;
    while (!stop)
    {
      {
        LockingEmitter::WriterLock lock(lockingEmitter_);
        if(func != NULL)
          (*func)(cookie);
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
        else if ((event.type == SDL_MOUSEMOTION) ||
          (event.type == SDL_MOUSEBUTTONDOWN) ||
          (event.type == SDL_MOUSEBUTTONUP))
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
        else if (event.type == SDL_MOUSEWHEEL)
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

          GuiAdapterWheelEvent dest;
          ConvertFromPlatform(dest, ctrlPressed, shiftPressed, altPressed, event);
          OnMouseWheelEvent(event.window.windowID, dest);

          //KeyboardModifiers modifiers = GetKeyboardModifiers(keyboardState, scancodeCount);

          //int x, y;
          //SDL_GetMouseState(&x, &y);

          //if (event.wheel.y > 0)
          //{
          //  locker.GetCentralViewport().MouseWheel(MouseWheelDirection_Up, x, y, modifiers);
          //}
          //else if (event.wheel.y < 0)
          //{
          //  locker.GetCentralViewport().MouseWheel(MouseWheelDirection_Down, x, y, modifiers);
          //}
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

          case SDLK_q:
            stop = true;
            break;

          default:
            GuiAdapterKeyboardEvent dest;
            ConvertFromPlatform(dest, event);
            OnKeyboardEvent(event.window.windowID, dest);
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

