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


#include "WorldSceneWidget.h"

#include "PanMouseTracker.h"
#include "ZoomMouseTracker.h"
#include "PanZoomMouseTracker.h"

#include <Logging.h>
#include <OrthancException.h>

#include <math.h>
#include <memory>
#include <cassert>

namespace Deprecated
{
  // this is an adapter between a IWorldSceneMouseTracker
  // that is tracking a mouse in scene coordinates/mm and
  // an IMouseTracker that is tracking a mouse
  // in screen coordinates/pixels.
  class WorldSceneWidget::SceneMouseTracker : public IMouseTracker
  {
  private:
    ViewportGeometry                        view_;
    std::unique_ptr<IWorldSceneMouseTracker>  tracker_;

  public:
    SceneMouseTracker(const ViewportGeometry& view,
                      IWorldSceneMouseTracker* tracker) :
      view_(view),
      tracker_(tracker)
    {
      if (tracker == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    virtual void Render(Orthanc::ImageAccessor& target)
    {
      if (tracker_->HasRender())
      {
        OrthancStone::CairoSurface surface(target, false /* no alpha */);
        OrthancStone::CairoContext context(surface);
        view_.ApplyTransform(context);
        tracker_->Render(context, view_.GetZoom());
      }
    }

    virtual void MouseUp()
    {
      tracker_->MouseUp();
    }

    virtual void MouseMove(int x,
                           int y,
                           const std::vector<Touch>& displayTouches)
    {
      double sceneX, sceneY;
      view_.MapPixelCenterToScene(sceneX, sceneY, x, y);

      std::vector<Touch> sceneTouches;
      for (size_t t = 0; t < displayTouches.size(); t++)
      {
        double sx, sy;
        
        view_.MapPixelCenterToScene(
          sx, sy, (int)displayTouches[t].x, (int)displayTouches[t].y);
        
        sceneTouches.push_back(
          Touch(static_cast<float>(sx), static_cast<float>(sy)));
      }
      tracker_->MouseMove(x, y, sceneX, sceneY, displayTouches, sceneTouches);
    }
  };


  bool WorldSceneWidget::RenderCairo(OrthancStone::CairoContext& context)
  {
    view_.ApplyTransform(context);
    return RenderScene(context, view_);
  }


  void WorldSceneWidget::RenderMouseOverCairo(OrthancStone::CairoContext& context,
                                              int x,
                                              int y)
  {
    ViewportGeometry view = GetView();
    view.ApplyTransform(context);

    double sceneX, sceneY;
    view.MapPixelCenterToScene(sceneX, sceneY, x, y);

    if (interactor_)
    {
      interactor_->MouseOver(context, *this, view, sceneX, sceneY, GetStatusBar());
    }
  }


  void WorldSceneWidget::SetSceneExtent(ViewportGeometry& view)
  {
    view.SetSceneExtent(GetSceneExtent());
  }


  void WorldSceneWidget::SetSize(unsigned int width,
                                 unsigned int height)
  {
    CairoWidget::SetSize(width, height);
    view_.SetDisplaySize(width, height);
  }


  void WorldSceneWidget::SetInteractor(IWorldSceneInteractor& interactor)
  {
    interactor_ = &interactor;
  }


  void WorldSceneWidget::FitContent()
  {
    SetSceneExtent(view_);
    view_.FitContent();

    NotifyContentChanged();
  }


  void WorldSceneWidget::SetView(const ViewportGeometry& view)
  {
    view_ = view;

    NotifyContentChanged();
  }


  IMouseTracker* WorldSceneWidget::CreateMouseTracker(OrthancStone::MouseButton button,
                                                      int x,
                                                      int y,
                                                      OrthancStone::KeyboardModifiers modifiers,
                                                      const std::vector<Touch>& touches)
  {
    double sceneX, sceneY;
    view_.MapPixelCenterToScene(sceneX, sceneY, x, y);

    // asks the Widget Interactor to provide a mouse tracker
    std::unique_ptr<IWorldSceneMouseTracker> tracker;

    if (interactor_)
    {
      tracker.reset(interactor_->CreateMouseTracker(*this, view_, button, modifiers, x, y, sceneX, sceneY, GetStatusBar(), touches));
    }
    
    if (tracker.get() != NULL)
    {
      return new SceneMouseTracker(view_, tracker.release());
    }
    else if (hasDefaultMouseEvents_)
    {
      if (touches.size() == 2)
      {
        return new SceneMouseTracker(view_, new PanZoomMouseTracker(*this, touches));
      }
      else
      {
        switch (button)
        {
          case OrthancStone::MouseButton_Middle:
            return new SceneMouseTracker(view_, new PanMouseTracker(*this, x, y));

          case OrthancStone::MouseButton_Right:
            return new SceneMouseTracker(view_, new ZoomMouseTracker(*this, x, y));

          default:
            return NULL;
        }
      }
    }
    else
    {
      return NULL;
    }
  }


  void WorldSceneWidget::MouseWheel(OrthancStone::MouseWheelDirection direction,
                                    int x,
                                    int y,
                                    OrthancStone::KeyboardModifiers modifiers)
  {
    if (interactor_)
    {
      interactor_->MouseWheel(*this, direction, modifiers, GetStatusBar());
    }
  }


  void WorldSceneWidget::KeyPressed(OrthancStone::KeyboardKeys key,
                                    char keyChar,
                                    OrthancStone::KeyboardModifiers modifiers)
  {
    if (interactor_)
    {
      interactor_->KeyPressed(*this, key, keyChar, modifiers, GetStatusBar());
    }
  }
}