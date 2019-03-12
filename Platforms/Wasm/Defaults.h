#pragma once

#include <emscripten/emscripten.h>

#include <Framework/dev.h>
#include <Framework/Viewport/WidgetViewport.h>
#include <Framework/Widgets/LayoutWidget.h>
#include <Applications/IStoneApplication.h>
#include <Platforms/Wasm/WasmPlatformApplicationAdapter.h>

typedef OrthancStone::WidgetViewport* ViewportHandle; // the objects exchanged between JS and C++

#ifdef __cplusplus
extern "C" {
#endif
  
  // JS methods accessible from C++
  extern void ScheduleWebViewportRedrawFromCpp(ViewportHandle cppViewportHandle);
  extern void UpdateStoneApplicationStatusFromCppWithString(const char* statusUpdateMessage);
  extern void UpdateStoneApplicationStatusFromCppWithSerializedMessage(const char* statusUpdateMessage);
  
  // C++ methods accessible from JS
  extern void EMSCRIPTEN_KEEPALIVE CreateWasmApplication(ViewportHandle cppViewportHandle);
  extern void EMSCRIPTEN_KEEPALIVE SetStartupParameter(const char* keyc, const char* value);
  

#ifdef __cplusplus
}
#endif

// these methods must be implemented in the custom app "mainWasm.cpp"
extern OrthancStone::IStoneApplication* CreateUserApplication(OrthancStone::MessageBroker& broker);
extern OrthancStone::WasmPlatformApplicationAdapter* CreateWasmApplicationAdapter(OrthancStone::MessageBroker& broker, OrthancStone::IStoneApplication* application);

namespace OrthancStone {

  // default Observer to trigger Viewport redraw when something changes in the Viewport
  class ViewportContentChangedObserver : public IObserver
  {
  private:
    // Flag to avoid flooding JavaScript with redundant Redraw requests
    bool isScheduled_; 

  public:
    ViewportContentChangedObserver(MessageBroker& broker) :
      IObserver(broker),
      isScheduled_(false)
    {
    }

    void Reset()
    {
      isScheduled_ = false;
    }

    void OnViewportChanged(const IViewport::ViewportChangedMessage& message)
    {
      if (!isScheduled_)
      {
        ScheduleWebViewportRedrawFromCpp((ViewportHandle)&message.GetOrigin());  // loosing constness when transmitted to Web
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
