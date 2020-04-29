/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#pragma once

#include "../../Framework/Viewport/WebAssemblyViewport.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2DViewport/UndoStack.h"
#include "../../Framework/Scene2DViewport/ViewportController.h"

#include <Core/OrthancException.h>

#include <emscripten/html5.h>
#include <boost/make_shared.hpp>

static const unsigned int FONT_SIZE = 32;

namespace OrthancStone
{
  class ActiveTracker : public boost::noncopyable
  {
  private:
    boost::shared_ptr<IFlexiblePointerTracker> tracker_;
    std::string                             canvasIdentifier_;
    bool                                    insideCanvas_;
    
  public:
    ActiveTracker(const boost::shared_ptr<IFlexiblePointerTracker>& tracker,
                  const std::string& canvasId) :
      tracker_(tracker),
      canvasIdentifier_(canvasId),
      insideCanvas_(true)
    {
      if (tracker_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    bool IsAlive() const
    {
      return tracker_->IsAlive();
    }

    void PointerMove(const PointerEvent& event)
    {
      tracker_->PointerMove(event);
    }

    void PointerUp(const PointerEvent& event)
    {
      tracker_->PointerUp(event);
    }
  };
}

static OrthancStone::PointerEvent* ConvertMouseEvent(
  const EmscriptenMouseEvent&        source,
  OrthancStone::IViewport& viewport)
{
  std::unique_ptr<OrthancStone::PointerEvent> target(
    new OrthancStone::PointerEvent);

  target->AddPosition(viewport.GetPixelCenterCoordinates(
                        source.targetX, source.targetY));
  target->SetAltModifier(source.altKey);
  target->SetControlModifier(source.ctrlKey);
  target->SetShiftModifier(source.shiftKey);

  return target.release();
}

std::unique_ptr<OrthancStone::ActiveTracker> tracker_;

EM_BOOL OnMouseEvent(int eventType, 
                     const EmscriptenMouseEvent *mouseEvent, 
                     void *userData)
{
  if (mouseEvent != NULL &&
      userData != NULL)
  {
    boost::shared_ptr<OrthancStone::ViewportController>& controller = 
      *reinterpret_cast<boost::shared_ptr<OrthancStone::ViewportController>*>(userData);

    switch (eventType)
    {
      case EMSCRIPTEN_EVENT_CLICK:
      {
        static unsigned int count = 0;
        char buf[64];
        sprintf(buf, "click %d", count++);

        std::unique_ptr<OrthancStone::TextSceneLayer> layer(new OrthancStone::TextSceneLayer);
        layer->SetText(buf);
        controller->GetViewport().GetScene().SetLayer(100, layer.release());
        controller->GetViewport().Refresh();
        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEDOWN:
      {
        boost::shared_ptr<OrthancStone::IFlexiblePointerTracker> t;

        {
          std::unique_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, controller->GetViewport()));

          switch (mouseEvent->button)
          {
            case 0:  // Left button
              emscripten_console_log("Creating RotateSceneTracker");
              t.reset(new OrthancStone::RotateSceneTracker(
                        controller, *event));
              break;

            case 1:  // Middle button
              emscripten_console_log("Creating PanSceneTracker");
              LOG(INFO) << "Creating PanSceneTracker" ;
              t.reset(new OrthancStone::PanSceneTracker(
                        controller, *event));
              break;

            case 2:  // Right button
              emscripten_console_log("Creating ZoomSceneTracker");
              t.reset(new OrthancStone::ZoomSceneTracker(
                        controller, *event, controller->GetViewport().GetCanvasWidth()));
              break;

            default:
              break;
          }
        }

        if (t.get() != NULL)
        {
          tracker_.reset(
            new OrthancStone::ActiveTracker(t, controller->GetViewport().GetCanvasIdentifier()));
          controller->GetViewport().Refresh();
        }

        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEMOVE:
        if (tracker_.get() != NULL)
        {
          std::unique_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, controller->GetViewport()));
          tracker_->PointerMove(*event);
          controller->GetViewport().Refresh();
        }
        break;

      case EMSCRIPTEN_EVENT_MOUSEUP:
        if (tracker_.get() != NULL)
        {
          std::unique_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, controller->GetViewport()));
          tracker_->PointerUp(*event);
          controller->GetViewport().Refresh();
          if (!tracker_->IsAlive())
            tracker_.reset();
        }
        break;

      default:
        break;
    }
  }

  return true;
}


void SetupEvents(const std::string& canvas,
                 boost::shared_ptr<OrthancStone::ViewportController>& controller)
{
  emscripten_set_mousedown_callback(canvas.c_str(), &controller, false, OnMouseEvent);
  emscripten_set_mousemove_callback(canvas.c_str(), &controller, false, OnMouseEvent);
  emscripten_set_mouseup_callback(canvas.c_str(), &controller, false, OnMouseEvent);
}
