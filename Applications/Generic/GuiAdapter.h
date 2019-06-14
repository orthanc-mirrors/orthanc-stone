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
#include <string>

#if ORTHANC_ENABLE_WASM != 1
# ifdef __EMSCRIPTEN__
#   error __EMSCRIPTEN__ is defined and ORTHANC_ENABLE_WASM != 1
# endif
#endif

#if ORTHANC_ENABLE_WASM == 1
# ifndef __EMSCRIPTEN__
#   error __EMSCRIPTEN__ is not defined and ORTHANC_ENABLE_WASM == 1
# endif
#endif

#if ORTHANC_ENABLE_WASM == 1
# include <emscripten/html5.h>
#else
# if ORTHANC_ENABLE_SDL == 1
#   include <SDL.h>
# endif
#endif

#include "../../Framework/StoneException.h"

#if ORTHANC_ENABLE_THREADS != 1
# include "../../Framework/Messages/LockingEmitter.h"
#endif

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace OrthancStone
{

  /**
  This interface is used to store the widgets that are controlled by the 
  GuiAdapter and receive event callbacks.
  The callbacks may possibly be downcast (using dynamic_cast, for safety) \
  to the actual widget type
  */
  class IGuiAdapterWidget
  {
  public:
    virtual ~IGuiAdapterWidget() {}
  };

  enum GuiAdapterMouseEventType
  {
    GUIADAPTER_EVENT_MOUSEDOWN = 1973,
    GUIADAPTER_EVENT_MOUSEMOVE = 1974,
    GUIADAPTER_EVENT_MOUSEUP   = 1975,
    GUIADAPTER_EVENT_WHEEL     = 1976
  };

  const unsigned int GUIADAPTER_DELTA_PIXEL = 2973;
  const unsigned int GUIADAPTER_DELTA_LINE  = 2974;
  const unsigned int GUIADAPTER_DELTA_PAGE  = 2975;

  struct GuiAdapterUiEvent;
  struct GuiAdapterMouseEvent;
  struct GuiAdapterWheelEvent;
  struct GuiAdapterKeyboardEvent;

  class LockingEmitter;
    
#if 1
  typedef bool (*OnMouseEventFunc)(const GuiAdapterMouseEvent* mouseEvent, void* userData);
  typedef bool (*OnMouseWheelFunc)(const GuiAdapterWheelEvent* wheelEvent, void* userData);
  typedef bool (*OnKeyDownFunc)   (const GuiAdapterKeyboardEvent*   keyEvent,   void* userData);
  typedef bool (*OnKeyUpFunc)     (const GuiAdapterKeyboardEvent*   keyEvent,   void* userData);

  typedef bool (*OnAnimationFrameFunc)(double time, void* userData);
  typedef bool (*OnWindowResizeFunc)(const GuiAdapterUiEvent* uiEvent, void* userData);

#else

#if ORTHANC_ENABLE_WASM == 1
  typedef EM_BOOL (*OnMouseEventFunc)(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData);
  typedef EM_BOOL (*OnMouseWheelFunc)(int eventType, const EmscriptenWheelEvent* wheelEvent, void* userData);
  typedef EM_BOOL (*OnKeyDownFunc)   (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData);
  typedef EM_BOOL (*OnKeyUpFunc)     (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData);

  typedef EM_BOOL (*OnAnimationFrameFunc)(double time, void* userData);
  typedef EM_BOOL (*OnWindowResizeFunc)(int eventType, const EmscriptenUiEvent* uiEvent, void* userData);
#else
  typedef bool    (*OnMouseEventFunc)(int eventType, const SDL_Event* mouseEvent, void* userData);
  typedef bool    (*OnMouseWheelFunc)(int eventType, const SDL_Event* wheelEvent, void* userData);
  typedef bool    (*OnKeyDownFunc)   (int eventType, const SDL_Event* keyEvent,   void* userData);
  typedef bool    (*OnKeyUpFunc)     (int eventType, const SDL_Event* keyEvent,   void* userData);

  typedef bool    (*OnAnimationFrameFunc)(double time, void* userData);
  typedef bool    (*OnWindowResizeFunc)(int eventType, const GuiAdapterUiEvent* uiEvent, void* userData);
#endif

#endif
  struct GuiAdapterMouseEvent {
    GuiAdapterMouseEventType type;
    //double                   timestamp;
    //long                     screenX;
    //long                     screenY;
    //long                     clientX;
    //long                     clientY;
    bool                     ctrlKey;
    bool                     shiftKey;
    bool                     altKey;
    //bool                     metaKey;
    unsigned short           button;
    //unsigned short           buttons;
    //long                     movementX;
    //long                     movementY;
    long                     targetX;
    long                     targetY;
    // canvasX and canvasY are deprecated - there no longer exists a Module['canvas'] object, so canvasX/Y are no longer reported (register a listener on canvas directly to get canvas coordinates, or translate manually)
    //long                     canvasX;
    //long                     canvasY;
    //long                     padding;
  };

  struct GuiAdapterWheelEvent {
    GuiAdapterMouseEvent mouse;
    double deltaX;
    double deltaY;
    unsigned long deltaMode;
  };

  // we don't use any data now
  struct GuiAdapterUiEvent {};

  // EmscriptenKeyboardEvent
  struct GuiAdapterKeyboardEvent
  {
    bool ctrlKey;
    bool shiftKey;
    bool altKey;
  };

  /*
    Mousedown event trigger when either the left or right (or middle) mouse is pressed 
    on the object;

    Mouseup event trigger when either the left or right (or middle) mouse is released
    above the object after triggered mousedown event and held.

    Click event trigger when the only left mouse button is pressed and released on the
    same object, requires the Mousedown and Mouseup event happened before Click event.

    The normal expect trigger order: onMousedown >> onMouseup >> onClick

    Testing in Chrome v58, the time between onMouseup and onClick events are around 
    7ms to 15ms

    FROM: https://codingrepo.com/javascript/2017/05/19/javascript-difference-mousedown-mouseup-click-events/
  */
#if ORTHANC_ENABLE_WASM == 1
  void ConvertFromPlatform(
    GuiAdapterUiEvent&       dest,
    int                      eventType,
    const EmscriptenUiEvent& src);

  void ConvertFromPlatform(
    GuiAdapterMouseEvent&       dest,
    int                         eventType,
    const EmscriptenMouseEvent& src);
  
  void ConvertFromPlatform(
    GuiAdapterWheelEvent&       dest,
    int                         eventType,
    const EmscriptenWheelEvent& src);

  void ConvertFromPlatform(
    GuiAdapterKeyboardEvent&       dest,
    const EmscriptenKeyboardEvent& src);

#else

# if ORTHANC_ENABLE_SDL == 1
  void ConvertFromPlatform(
    GuiAdapterMouseEvent& dest,
    bool ctrlPressed, bool shiftPressed, bool altPressed,
    const SDL_Event& source);
# endif

#endif

  class GuiAdapter
  {
  public:
#if ORTHANC_ENABLE_THREADS == 1
    GuiAdapter(LockingEmitter& lockingEmitter) : lockingEmitter_(lockingEmitter)
    {}
#else
    GuiAdapter() {}
#endif

    void RegisterWidget(boost::shared_ptr<IGuiAdapterWidget> widget);
    
    /**
      emscripten_set_resize_callback("#window", NULL, false, OnWindowResize);

      emscripten_set_wheel_callback("mycanvas1", widget1_.get(), false, OnMouseWheel);
      emscripten_set_wheel_callback("mycanvas2", widget2_.get(), false, OnMouseWheel);
      emscripten_set_wheel_callback("mycanvas3", widget3_.get(), false, OnMouseWheel);

      emscripten_set_keydown_callback("#window", NULL, false, OnKeyDown);
      emscripten_set_keyup_callback("#window", NULL, false, OnKeyUp);

      emscripten_request_animation_frame_loop(OnAnimationFrame, NULL);
    
      SDL:
      see https://wiki.libsdl.org/SDL_CaptureMouse

    */

    void SetMouseDownCallback(std::string canvasId, void* userData, bool capture, OnMouseEventFunc   func);
    void SetMouseMoveCallback(std::string canvasId, void* userData, bool capture, OnMouseEventFunc   func);
    void SetMouseUpCallback  (std::string canvasId, void* userData, bool capture, OnMouseEventFunc   func);
    void SetWheelCallback    (std::string canvasId, void* userData, bool capture, OnMouseWheelFunc   func);
    void SetKeyDownCallback  (std::string canvasId, void* userData, bool capture, OnKeyDownFunc      func);
    void SetKeyUpCallback    (std::string canvasId, void* userData, bool capture, OnKeyUpFunc        func);
    
    // if you pass "#window", under SDL, then any Window resize will trigger the callback
    void SetResizeCallback (std::string canvasId, void* userData, bool capture, OnWindowResizeFunc func);

    void RequestAnimationFrame(OnAnimationFrameFunc func, void* userData);

    // TODO: implement and call to remove canvases [in SDL, although code should be generic]
    void SetOnExitCallback();

    // void 
    // OnWindowResize
    // oracle
    // broker

    /**
    Under SDL, this function does NOT return until all windows have been closed.
    Under wasm, it returns without doing anything, since the event loop is managed
    by the browser.
    */
    void Run();

#if ORTHANC_ENABLE_WASM != 1
    /**
    This method must be called in order for callback handler to be allowed to 
    be registered.

    We'll retrieve its name and use it as the canvas name in all subsequent 
    calls
    */
    //void RegisterSdlWindow(SDL_Window* window);
    //void UnregisterSdlWindow(SDL_Window* window);
#endif

  private:

#if ORTHANC_ENABLE_THREADS == 1
    /**
    This object is used by the multithreaded Oracle to serialize access to
    shared data. We need to use it as soon as we access the state.
    */
    LockingEmitter& lockingEmitter_;
#endif

    /**
    In SDL, this executes all the registered headers
    */
    void OnAnimationFrame();

    //void RequestAnimationFrame(OnAnimationFrameFunc func, void* userData);
    std::vector<std::pair<OnAnimationFrameFunc, void*> >  
      animationFrameHandlers_;
    
    void OnResize();

    std::vector<std::pair<OnWindowResizeFunc, void*> >
      resizeHandlers_;
    

#if ORTHANC_ENABLE_SDL == 1

    /**
    This executes all the registered headers if needed (in wasm, the browser
    deals with this)
    */
    void OnMouseEvent(uint32_t windowID, const GuiAdapterMouseEvent& event);
#endif

    /**
    This executes all the registered headers if needed (in wasm, the browser
    deals with this)
    */
    void ViewportsUpdateSize();

    std::vector<boost::weak_ptr<IGuiAdapterWidget>> widgets_;

    template<typename F> void VisitWidgets(F func)
    {
      for (auto w : widgets_)
      {
        boost::shared_ptr<IGuiAdapterWidget> widget = w.lock();
        func(widget);
      }
    }
  };
}
