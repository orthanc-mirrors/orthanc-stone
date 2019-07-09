#pragma once

#include "../../Applications/Generic/Scene2DInteractor.h"
#include "../../Framework/Scene2DViewport/IFlexiblePointerTracker.h"


class BasicScene2DInteractor : public OrthancStone::Scene2DInteractor
{
  boost::shared_ptr<OrthancStone::IFlexiblePointerTracker>  currentTracker_;
public:
  BasicScene2DInteractor(boost::shared_ptr<OrthancStone::ViewportController> viewportController) :
    Scene2DInteractor(viewportController)
  {}

  virtual bool OnMouseEvent(const OrthancStone::GuiAdapterMouseEvent& event, const OrthancStone::PointerEvent& pointerEvent) override;
  virtual bool OnKeyboardEvent(const OrthancStone::GuiAdapterKeyboardEvent& guiEvent);
  virtual bool OnWheelEvent(const OrthancStone::GuiAdapterWheelEvent& guiEvent);
};

