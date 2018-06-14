#pragma once

#include <emscripten/emscripten.h>

#include <Framework/dev.h>
#include <Framework/Viewport/WidgetViewport.h>
#include <Framework/Widgets/LayerWidget.h>
#include <Framework/Widgets/LayoutWidget.h>
#include <Applications/Wasm/BasicWasmApplication.h>
#include <Applications/Wasm/BasicWasmApplicationContext.h>

typedef OrthancStone::IViewport* ViewportHandle; // the objects exchanged between JS and C++

#ifdef __cplusplus
extern "C" {
#endif
  
  // JS methods accessible from C++
  extern void ScheduleWebViewportRedrawFromCpp(ViewportHandle cppViewportHandle);
  
  // C++ methods accessible from JS
  extern void EMSCRIPTEN_KEEPALIVE CreateWasmApplication(ViewportHandle cppViewportHandle);

#ifdef __cplusplus
}
#endif

extern OrthancStone::BasicWasmApplication* CreateUserApplication();

namespace OrthancStone {

  // default Ovserver to trigger Viewport redraw when something changes in the Viewport
  class ChangeObserver :
    public OrthancStone::IViewport::IObserver
  {
  private:
    // Flag to avoid flooding JavaScript with redundant Redraw requests
    bool isScheduled_; 

  public:
    ChangeObserver() :
      isScheduled_(false)
    {
    }

    void Reset()
    {
      isScheduled_ = false;
    }

    virtual void NotifyChange(const OrthancStone::IViewport &viewport)
    {
      if (!isScheduled_)
      {
        ScheduleWebViewportRedrawFromCpp((ViewportHandle)&viewport);  // loosing constness when transmitted to Web
        isScheduled_ = true;
      }
    }
  };

  // default status bar to log messages on the console/stdout
  class StatusBar : public OrthancStone::IStatusBar
  {
  public:
    virtual void ClearMessage()
    {
    }

    virtual void SetMessage(const std::string& message)
    {
      printf("%s\n", message.c_str());
    }
  };
}