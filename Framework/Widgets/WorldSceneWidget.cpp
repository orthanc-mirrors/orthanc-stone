/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include <math.h>
#include <memory>
#include <cassert>
#include "Core/Logging.h"

namespace OrthancStone
{
  static void MapMouseToScene(double& sceneX,
                              double& sceneY,
                              const ViewportGeometry& view,
                              int mouseX,
                              int mouseY)
  {
    // Take the center of the pixel
    double x, y;
    x = static_cast<double>(mouseX) + 0.5;
    y = static_cast<double>(mouseY) + 0.5;

    view.MapDisplayToScene(sceneX, sceneY, x, y);
  }


  // this is an adapter between a IWorldSceneMouseTracker
  // that is tracking a mouse in scene coordinates/mm and
  // an IMouseTracker that is tracking a mouse
  // in screen coordinates/pixels.
  class WorldSceneWidget::SceneMouseTracker : public IMouseTracker
  {
  private:
    ViewportGeometry                       view_;
    std::auto_ptr<IWorldSceneMouseTracker>  tracker_;

  public:
    SceneMouseTracker(const ViewportGeometry& view,
                      IWorldSceneMouseTracker* tracker) :
      view_(view),
      tracker_(tracker)
    {
      assert(tracker != NULL);
    }

    virtual void Render(Orthanc::ImageAccessor& target)
    {
      CairoSurface surface(target);
      CairoContext context(surface);
      view_.ApplyTransform(context);
      tracker_->Render(context, view_.GetZoom());
    }

    virtual void MouseUp()
    {
      tracker_->MouseUp();
    }

    virtual void MouseMove(int x,
                           int y)
    {
      double sceneX, sceneY;
      MapMouseToScene(sceneX, sceneY, view_, x, y);
      tracker_->MouseMove(sceneX, sceneY);
    }
  };


  WorldSceneWidget::PanMouseTracker::PanMouseTracker(WorldSceneWidget& that,
                                                     int x,
                                                     int y) :
    that_(that),
    downX_(x),
    downY_(y)
  {
    that_.view_.GetPan(previousPanX_, previousPanY_);
  }

  
  void WorldSceneWidget::PanMouseTracker::MouseMove(int x, int y)
  {
    that_.view_.SetPan(previousPanX_ + x - downX_,
                       previousPanY_ + y - downY_);
  }

  
  WorldSceneWidget::ZoomMouseTracker::ZoomMouseTracker(WorldSceneWidget&  that,
                                                       int x,
                                                       int y) :
    that_(that),
    downX_(x),
    downY_(y)
  {
    oldZoom_ = that_.view_.GetZoom();
    MapMouseToScene(centerX_, centerY_, that_.view_, downX_, downY_);
  }

  void WorldSceneWidget::ZoomMouseTracker::MouseMove(int x,
                                                     int y)
  {
    static const double MIN_ZOOM = -4;
    static const double MAX_ZOOM = 4;

    if (that_.view_.GetDisplayHeight() <= 3)
    {
      LOG(WARNING) << "image is too small to zoom (current height = " << that_.view_.GetDisplayHeight() << ")";
      return;
    }

    double dy = (static_cast<double>(y - downY_) /
                 static_cast<double>(that_.view_.GetDisplayHeight() - 1)); // In the range [-1,1]
    double z;

    // Linear interpolation from [-1, 1] to [MIN_ZOOM, MAX_ZOOM]
    if (dy < -1.0)
    {
      z = MIN_ZOOM;
    }
    else if (dy > 1.0)
    {
      z = MAX_ZOOM;
    }
    else
    {
      z = MIN_ZOOM + (MAX_ZOOM - MIN_ZOOM) * (dy + 1.0) / 2.0;
    }

    z = pow(2.0, z);

    that_.view_.SetZoom(oldZoom_ * z);

    // Correct the pan so that the original click point is kept at
    // the same location on the display
    double panX, panY;
    that_.view_.GetPan(panX, panY);

    int tx, ty;
    that_.view_.MapSceneToDisplay(tx, ty, centerX_, centerY_);
    that_.view_.SetPan(panX + static_cast<double>(downX_ - tx),
                       panY + static_cast<double>(downY_ - ty));
  }


  bool WorldSceneWidget::RenderCairo(CairoContext& context)
  {
    view_.ApplyTransform(context);
    return RenderScene(context, view_);
  }


  void WorldSceneWidget::RenderMouseOverCairo(CairoContext& context,
                                              int x,
                                              int y)
  {
    ViewportGeometry view = GetView();
    view.ApplyTransform(context);

    double sceneX, sceneY;
    MapMouseToScene(sceneX, sceneY, view, x, y);
    RenderSceneMouseOver(context, view, sceneX, sceneY);
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


  ViewportGeometry WorldSceneWidget::GetView()
  {
    return view_;
  }


  IMouseTracker* WorldSceneWidget::CreateMouseTracker(MouseButton button,
                                                      int x,
                                                      int y,
                                                      KeyboardModifiers modifiers)
  {
    double sceneX, sceneY;
    MapMouseToScene(sceneX, sceneY, view_, x, y);

    // asks the Widget Interactor to provide a mouse tracker
    std::auto_ptr<IWorldSceneMouseTracker> tracker
        (CreateMouseSceneTracker(view_, button, sceneX, sceneY, modifiers));

    if (tracker.get() != NULL)
    {
      return new SceneMouseTracker(view_, tracker.release());
    }
    //TODO: allow Interactor to create Pan & Zoom
    switch (button)
    {
    case MouseButton_Middle:
      return new PanMouseTracker(*this, x, y);

    case MouseButton_Right:
      return new ZoomMouseTracker(*this, x, y);

    default:
      return NULL;
    }
  }


  void WorldSceneWidget::RenderSceneMouseOver(CairoContext& context,
                                              const ViewportGeometry& view,
                                              double x,
                                              double y)
  {
    if (interactor_)
    {
      interactor_->MouseOver(context, *this, view, x, y, GetStatusBar());
    }
  }

  IWorldSceneMouseTracker* WorldSceneWidget::CreateMouseSceneTracker(const ViewportGeometry& view,
                                                                     MouseButton button,
                                                                     double x,
                                                                     double y,
                                                                     KeyboardModifiers modifiers)
  {
    if (interactor_)
    {
      return interactor_->CreateMouseTracker(*this, view, button, modifiers, x, y, GetStatusBar());
    }
    else
    {
      return NULL;
    }
  }


  void WorldSceneWidget::MouseWheel(MouseWheelDirection direction,
                                    int x,
                                    int y,
                                    KeyboardModifiers modifiers)
  {
    if (interactor_)
    {
      interactor_->MouseWheel(*this, direction, modifiers, GetStatusBar());
    }
  }


  void WorldSceneWidget::KeyPressed(KeyboardKeys key,
                                    char keyChar,
                                    KeyboardModifiers modifiers)
  {
    if (interactor_)
    {
      interactor_->KeyPressed(*this, key, keyChar, modifiers, GetStatusBar());
    }
  }
}
