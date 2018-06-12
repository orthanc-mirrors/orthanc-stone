#pragma once

#include <Framework/dev.h>
#include <Framework/Viewport/WidgetViewport.h>
#include <Framework/Widgets/LayerWidget.h>
#include <Framework/Widgets/LayoutWidget.h>

#ifdef __cplusplus
extern "C" {
#endif
  
  extern void ScheduleRedraw();
  
#ifdef __cplusplus
}
#endif


namespace OrthancStone {

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

    virtual void NotifyChange(const OrthancStone::IViewport &scene)
    {
      if (!isScheduled_)
      {
        ScheduleRedraw();
        isScheduled_ = true;
      }
    }
  };


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