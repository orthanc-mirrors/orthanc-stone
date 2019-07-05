#pragma once

#include "../../Framework/Scene2DViewport/ViewportController.h"
#include "../../Applications/Generic/GuiAdapter.h"


namespace OrthancStone
{

  class Scene2DInteractor
  {
  protected:
    boost::shared_ptr<ViewportController> viewportController_;

  public:
    Scene2DInteractor(boost::shared_ptr<ViewportController> viewportController) :
      viewportController_(viewportController)
    {}

    virtual void OnMouseEvent(const GuiAdapterMouseEvent& event) = 0;
  };
}

class BasicScene2DInteractor : public OrthancStone::Scene2DInteractor
{
public:
  BasicScene2DInteractor(boost::shared_ptr<OrthancStone::ViewportController> viewportController) :
    Scene2DInteractor(viewportController)
  {}

  virtual void OnMouseEvent(const OrthancStone::GuiAdapterMouseEvent& event) override;
};

