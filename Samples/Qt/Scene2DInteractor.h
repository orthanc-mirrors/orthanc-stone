#pragma once

#include "../../Framework/Scene2D/PointerEvent.h"
#include "../../Framework/Scene2DViewport/ViewportController.h"
#include "../../Framework/Scene2DViewport/IFlexiblePointerTracker.h"
#include "../../Framework/Scene2D/Internals/CompositorHelper.h"
#include "../../Applications/Generic/GuiAdapter.h"


namespace OrthancStone
{

  class Scene2DInteractor
  {
  protected:
    boost::shared_ptr<ViewportController>       viewportController_;
    boost::shared_ptr<IFlexiblePointerTracker>  currentTracker_;
    boost::shared_ptr<ICompositor>              compositor_;

  public:
    Scene2DInteractor(boost::shared_ptr<ViewportController> viewportController) :
      viewportController_(viewportController)
    {}

    void SetCompositor(boost::shared_ptr<ICompositor> compositor)
    {
      compositor_ = compositor;
    }

    virtual bool OnMouseEvent(const GuiAdapterMouseEvent& guiEvent, const PointerEvent& pointerEvent) = 0; // returns true if it has handled the event
    virtual bool OnKeyboardEvent(const GuiAdapterKeyboardEvent& guiEvent) = 0; // returns true if it has handled the event
    virtual bool OnWheelEvent(const GuiAdapterWheelEvent& guiEvent) = 0; // returns true if it has handled the event

  };
}




class BasicScene2DInteractor : public OrthancStone::Scene2DInteractor
{
public:
  BasicScene2DInteractor(boost::shared_ptr<OrthancStone::ViewportController> viewportController) :
    Scene2DInteractor(viewportController)
  {}

  virtual bool OnMouseEvent(const OrthancStone::GuiAdapterMouseEvent& event, const OrthancStone::PointerEvent& pointerEvent) override;
  virtual bool OnKeyboardEvent(const OrthancStone::GuiAdapterKeyboardEvent& guiEvent);
  virtual bool OnWheelEvent(const OrthancStone::GuiAdapterWheelEvent& guiEvent);
};

