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


#pragma once

#include "../../Framework/OpenGL/WebAssemblyOpenGLContext.h"
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
  class WebAssemblyViewport : public boost::noncopyable
  {
  private:
    // the construction order is important because compositor_
    // will hold a reference to the scene that belong to the 
    // controller_ object
    OpenGL::WebAssemblyOpenGLContext       context_;
    boost::shared_ptr<ViewportController>  controller_;
    OpenGLCompositor                       compositor_;

    void SetupEvents(const std::string& canvas);

  public:
    WebAssemblyViewport(MessageBroker& broker,
                        const std::string& canvas) :
      context_(canvas),
      controller_(new ViewportController(boost::make_shared<UndoStack>(), broker)),
      compositor_(context_, *controller_->GetScene())
    {
      compositor_.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, 
                          FONT_SIZE, Orthanc::Encoding_Latin1);
      SetupEvents(canvas);
    }

    Scene2D& GetScene()
    {
      return *controller_->GetScene();
    }

    const boost::shared_ptr<ViewportController>& GetController()
    {
      return controller_;
    }

    void UpdateSize()
    {
      context_.UpdateSize();
      //compositor_.UpdateSize();
      Refresh();
    }

    void Refresh()
    {
      compositor_.Refresh();
    }

    void FitContent()
    {
      GetScene().FitContent(context_.GetCanvasWidth(), context_.GetCanvasHeight());
    }

    const std::string& GetCanvasIdentifier() const
    {
      return context_.GetCanvasIdentifier();
    }

    ScenePoint2D GetPixelCenterCoordinates(int x, int y) const
    {
      return compositor_.GetPixelCenterCoordinates(x, y);
    }

    unsigned int GetCanvasWidth() const
    {
      return context_.GetCanvasWidth();
    }

    unsigned int GetCanvasHeight() const
    {
      return context_.GetCanvasHeight();
    }
  };

  class ActiveTracker : public boost::noncopyable
  {
  private:
    boost::shared_ptr<IFlexiblePointerTracker> tracker_;
    std::string                             canvasIdentifier_;
    bool                                    insideCanvas_;
    
  public:
    ActiveTracker(const boost::shared_ptr<IFlexiblePointerTracker>& tracker,
                  const WebAssemblyViewport& viewport) :
      tracker_(tracker),
      canvasIdentifier_(viewport.GetCanvasIdentifier()),
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
  OrthancStone::WebAssemblyViewport& viewport)
{
  std::auto_ptr<OrthancStone::PointerEvent> target(
    new OrthancStone::PointerEvent);

  target->AddPosition(viewport.GetPixelCenterCoordinates(
    source.targetX, source.targetY));
  target->SetAltModifier(source.altKey);
  target->SetControlModifier(source.ctrlKey);
  target->SetShiftModifier(source.shiftKey);

  return target.release();
}

std::auto_ptr<OrthancStone::ActiveTracker> tracker_;

EM_BOOL OnMouseEvent(int eventType, 
                     const EmscriptenMouseEvent *mouseEvent, 
                     void *userData)
{
  if (mouseEvent != NULL &&
      userData != NULL)
  {
    OrthancStone::WebAssemblyViewport& viewport = 
      *reinterpret_cast<OrthancStone::WebAssemblyViewport*>(userData);

    switch (eventType)
    {
      case EMSCRIPTEN_EVENT_CLICK:
      {
        static unsigned int count = 0;
        char buf[64];
        sprintf(buf, "click %d", count++);

        std::auto_ptr<OrthancStone::TextSceneLayer> layer(new OrthancStone::TextSceneLayer);
        layer->SetText(buf);
        viewport.GetScene().SetLayer(100, layer.release());
        viewport.Refresh();
        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEDOWN:
      {
        boost::shared_ptr<OrthancStone::IFlexiblePointerTracker> t;

        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));

          switch (mouseEvent->button)
          {
            case 0:  // Left button
              emscripten_console_log("Creating RotateSceneTracker");
              t.reset(new OrthancStone::RotateSceneTracker(
                viewport.GetController(), *event));
              break;

            case 1:  // Middle button
              emscripten_console_log("Creating PanSceneTracker");
              LOG(INFO) << "Creating PanSceneTracker" ;
              t.reset(new OrthancStone::PanSceneTracker(
                viewport.GetController(), *event));
              break;

            case 2:  // Right button
              emscripten_console_log("Creating ZoomSceneTracker");
              t.reset(new OrthancStone::ZoomSceneTracker(
                viewport.GetController(), *event, viewport.GetCanvasWidth()));
              break;

            default:
              break;
          }
        }

        if (t.get() != NULL)
        {
          tracker_.reset(
            new OrthancStone::ActiveTracker(t, viewport));
          viewport.Refresh();
        }

        break;
      }

      case EMSCRIPTEN_EVENT_MOUSEMOVE:
        if (tracker_.get() != NULL)
        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));
          tracker_->PointerMove(*event);
          viewport.Refresh();
        }
        break;

      case EMSCRIPTEN_EVENT_MOUSEUP:
        if (tracker_.get() != NULL)
        {
          std::auto_ptr<OrthancStone::PointerEvent> event(
            ConvertMouseEvent(*mouseEvent, viewport));
          tracker_->PointerUp(*event);
          viewport.Refresh();
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


void OrthancStone::WebAssemblyViewport::SetupEvents(const std::string& canvas)
{
  emscripten_set_mousedown_callback(canvas.c_str(), this, false, OnMouseEvent);
  emscripten_set_mousemove_callback(canvas.c_str(), this, false, OnMouseEvent);
  emscripten_set_mouseup_callback(canvas.c_str(), this, false, OnMouseEvent);
}
