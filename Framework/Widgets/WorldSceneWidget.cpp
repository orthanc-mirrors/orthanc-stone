/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "../../Resources/Orthanc/Core/OrthancException.h"

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


  struct WorldSceneWidget::ViewChangeFunctor
  {
    const ViewportGeometry& view_;

    ViewChangeFunctor(const ViewportGeometry& view) :
      view_(view)
    {
    }

    void operator() (IWorldObserver& observer,
                     const WorldSceneWidget& source)
    {
      observer.NotifyViewChange(source, view_);
    }
  };


  struct WorldSceneWidget::SizeChangeFunctor
  {
    ViewportGeometry& view_;

    SizeChangeFunctor(ViewportGeometry& view) :
      view_(view)
    {
    }

    void operator() (IWorldObserver& observer,
                     const WorldSceneWidget& source)
    {
      observer.NotifySizeChange(source, view_);
    }
  };


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


  class WorldSceneWidget::PanMouseTracker : public IMouseTracker
  {
  private:
    WorldSceneWidget&  that_;  
    double             previousPanX_;
    double             previousPanY_;
    double             downX_;
    double             downY_;

  public:
    PanMouseTracker(WorldSceneWidget& that,
                    int x,
                    int y) :
      that_(that),
      downX_(x),
      downY_(y)
    {
      SharedValue<ViewportGeometry>::Locker locker(that_.view_);
      locker.GetValue().GetPan(previousPanX_, previousPanY_);
    }

    virtual void Render(Orthanc::ImageAccessor& surface)
    {
    }

    virtual void MouseUp() 
    {
    }

    virtual void MouseMove(int x, 
                           int y)
    {
      SharedValue<ViewportGeometry>::Locker locker(that_.view_);
      locker.GetValue().SetPan(previousPanX_ + x - downX_,
                               previousPanY_ + y - downY_);

      ViewChangeFunctor functor(locker.GetValue());
      that_.observers_.Notify(&that_, functor);
    }
  };


  class WorldSceneWidget::ZoomMouseTracker : public IMouseTracker
  {
  private:
    WorldSceneWidget&  that_;  
    int                downX_;
    int                downY_;
    double             centerX_;
    double             centerY_;
    double             oldZoom_;

  public:
    ZoomMouseTracker(WorldSceneWidget&  that,
                     int x,
                     int y) :
      that_(that),
      downX_(x),
      downY_(y)
    {
      SharedValue<ViewportGeometry>::Locker locker(that_.view_);
      oldZoom_ = locker.GetValue().GetZoom();
      MapMouseToScene(centerX_, centerY_, locker.GetValue(), downX_, downY_);
    }

    virtual void Render(Orthanc::ImageAccessor& surface)
    {
    }

    virtual void MouseUp() 
    {
    }

    virtual void MouseMove(int x, 
                           int y)
    {
      static const double MIN_ZOOM = -4;
      static const double MAX_ZOOM = 4;

      SharedValue<ViewportGeometry>::Locker locker(that_.view_);

      if (locker.GetValue().GetDisplayHeight() <= 3)
      {
        return;   // Cannot zoom on such a small image
      }

      double dy = (static_cast<double>(y - downY_) / 
                   static_cast<double>(locker.GetValue().GetDisplayHeight() - 1)); // In the range [-1,1]
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

      locker.GetValue().SetZoom(oldZoom_ * z);

      // Correct the pan so that the original click point is kept at
      // the same location on the display
      double panX, panY;
      locker.GetValue().GetPan(panX, panY);

      int tx, ty;
      locker.GetValue().MapSceneToDisplay(tx, ty, centerX_, centerY_);
      locker.GetValue().SetPan(panX + static_cast<double>(downX_ - tx),
                               panY + static_cast<double>(downY_ - ty));

      ViewChangeFunctor functor(locker.GetValue());
      that_.observers_.Notify(&that_, functor);
    }
  };


  bool WorldSceneWidget::RenderCairo(CairoContext& context)
  {
    ViewportGeometry view;

    {
      SharedValue<ViewportGeometry>::Locker locker(view_);
      view = locker.GetValue();
    }

    view.ApplyTransform(context);

    return RenderScene(context, view);
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


  void WorldSceneWidget::SetSceneExtent(SharedValue<ViewportGeometry>::Locker& locker)
  {
    double x1, y1, x2, y2;
    GetSceneExtent(x1, y1, x2, y2);
    locker.GetValue().SetSceneExtent(x1, y1, x2, y2);
  }


  void WorldSceneWidget::SetSize(unsigned int width,
                                 unsigned int height)
  {
    CairoWidget::SetSize(width, height);

    {
      SharedValue<ViewportGeometry>::Locker locker(view_);
      locker.GetValue().SetDisplaySize(width, height);

      if (observers_.IsEmpty())
      {
        // Without a size observer, use the default view
        locker.GetValue().SetDefaultView();
      }
      else
      {
        // With a size observer, let it decide which view to use
        SizeChangeFunctor functor(locker.GetValue());
        observers_.Notify(this, functor);
      }
    }
  }


  void WorldSceneWidget::SetInteractor(IWorldSceneInteractor& interactor)
  {
    if (IsStarted())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    interactor_ = &interactor;
  }


  void WorldSceneWidget::Start()
  {
    ViewportGeometry geometry;

    {
      SharedValue<ViewportGeometry>::Locker locker(view_);
      SetSceneExtent(locker);
      geometry = locker.GetValue();
    }

    WidgetBase::Start();

    ViewChangeFunctor functor(geometry);
    observers_.Notify(this, functor);
  }
      

  void WorldSceneWidget::SetDefaultView()
  {
    ViewportGeometry geometry;

    {
      SharedValue<ViewportGeometry>::Locker locker(view_);
      SetSceneExtent(locker);
      locker.GetValue().SetDefaultView();
      geometry = locker.GetValue();
    }

    NotifyChange();

    ViewChangeFunctor functor(geometry);
    observers_.Notify(this, functor);
  }


  void WorldSceneWidget::SetView(const ViewportGeometry& view)
  {
    {
      SharedValue<ViewportGeometry>::Locker locker(view_);
      locker.GetValue() = view;
    }

    NotifyChange();

    ViewChangeFunctor functor(view);
    observers_.Notify(this, functor);
  }


  ViewportGeometry WorldSceneWidget::GetView()
  {
    SharedValue<ViewportGeometry>::Locker locker(view_);
    return locker.GetValue();
  }


  IMouseTracker* WorldSceneWidget::CreateMouseTracker(MouseButton button,
                                                      int x,
                                                      int y,
                                                      KeyboardModifiers modifiers)
  {
    ViewportGeometry view = GetView();

    double sceneX, sceneY;
    MapMouseToScene(sceneX, sceneY, view, x, y);

    std::auto_ptr<IWorldSceneMouseTracker> tracker(CreateMouseSceneTracker(view, button, sceneX, sceneY, modifiers));
    if (tracker.get() != NULL)
    {
      return new SceneMouseTracker(view, tracker.release());
    }

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
      interactor_->MouseOver(context, *this, GetSlice(), view, x, y, GetStatusBar());
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
      return interactor_->CreateMouseTracker(*this, GetSlice(), view, button, x, y, GetStatusBar());
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


  void WorldSceneWidget::KeyPressed(char key,
                                    KeyboardModifiers modifiers)
  {
    if (interactor_)
    {
      interactor_->KeyPressed(*this, key, modifiers, GetStatusBar());
    }
  }
}
