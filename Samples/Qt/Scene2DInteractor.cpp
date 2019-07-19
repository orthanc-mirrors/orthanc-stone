#include "Scene2DInteractor.h"

#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"


namespace OrthancStone
{

}

using namespace OrthancStone;


bool BasicScene2DInteractor::OnMouseEvent(const GuiAdapterMouseEvent& event, const PointerEvent& pointerEvent)
{
  if (currentTracker_.get() != NULL)
  {
    switch (event.type)
    {
    case GUIADAPTER_EVENT_MOUSEUP:
    {
      currentTracker_->PointerUp(pointerEvent);
      if (!currentTracker_->IsAlive())
      {
        currentTracker_.reset();
      }
    };break;
    case GUIADAPTER_EVENT_MOUSEMOVE:
    {
      currentTracker_->PointerMove(pointerEvent);
    };break;
    }
    return true;
  }
  else
  {
    if (event.button == GUIADAPTER_MOUSEBUTTON_LEFT)
    {
      currentTracker_.reset(new RotateSceneTracker(viewportController_, pointerEvent));
    }
    else if (event.button == GUIADAPTER_MOUSEBUTTON_MIDDLE)
    {
      currentTracker_.reset(new PanSceneTracker(viewportController_, pointerEvent));
    }
    else if (event.button == GUIADAPTER_MOUSEBUTTON_RIGHT && compositor_.get() != NULL)
    {
      currentTracker_.reset(new ZoomSceneTracker(viewportController_, pointerEvent, compositor_->GetHeight()));
    }
    return true;
  }
  return false;
}

bool BasicScene2DInteractor::OnKeyboardEvent(const GuiAdapterKeyboardEvent& guiEvent)
{
  switch (guiEvent.sym[0])
  {
  case 's':
  {
    viewportController_->FitContent(compositor_->GetWidth(), compositor_->GetHeight());
    return true;
  };
  }
  return false;
}

bool BasicScene2DInteractor::OnWheelEvent(const GuiAdapterWheelEvent& guiEvent)
{
  return false;
}
