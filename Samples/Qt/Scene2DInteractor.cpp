#include "Scene2DInteractor.h"

#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"


namespace OrthancStone
{

}

using namespace OrthancStone;


void BasicScene2DInteractor::OnMouseEvent(const GuiAdapterMouseEvent& event, const PointerEvent& pointerEvent)
{
  if (currentTracker_.get() != NULL)
  {
    currentTracker_->PointerMove(pointerEvent);
  }
  else
  {
    if (event.button == GUIADAPTER_MOUSEBUTTON_LEFT)
    {
    }
    else if (event.button == GUIADAPTER_MOUSEBUTTON_MIDDLE)
    {
      currentTracker_.reset(new PanSceneTracker(viewportController_, pointerEvent));
    }
    else if (event.button == GUIADAPTER_MOUSEBUTTON_RIGHT)
    {
     // TODO: need a pointer to compositor currentTracker_.reset(new ZoomSceneTracker(viewportController_, pointerEvent, viewportController_->));
    }
  }

}


