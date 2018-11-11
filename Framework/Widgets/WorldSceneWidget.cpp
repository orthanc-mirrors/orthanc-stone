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

#include "PanMouseTracker.h"
#include "ZoomMouseTracker.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <math.h>
#include <memory>
#include <cassert>

namespace OrthancStone
{
  // this is an adapter between a IWorldSceneMouseTracker
  // that is tracking a mouse in scene coordinates/mm and
  // an IMouseTracker that is tracking a mouse
  // in screen coordinates/pixels.
  class WorldSceneWidget::SceneMouseTracker : public IMouseTracker
  {
  private:
    ViewportGeometry                        view_;
    std::auto_ptr<IWorldSceneMouseTracker>  tracker_;

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
        CairoSurface surface(target);
        CairoContext context(surface);
        view_.ApplyTransform(context);
        tracker_->Render(context, view_.GetZoom());
      }
    }

    virtual void MouseUp()
    {
      tracker_->MouseUp();
    }

    virtual void MouseMove(int x,
                           int y)
    {
      double sceneX, sceneY;
      view_.MapPixelCenterToScene(sceneX, sceneY, x, y);
      tracker_->MouseMove(x, y, sceneX, sceneY);
    }
  };


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


  IMouseTracker* WorldSceneWidget::CreateMouseTracker(MouseButton button,
                                                      int x,
                                                      int y,
                                                      KeyboardModifiers modifiers)
  {
    double sceneX, sceneY;
    view_.MapPixelCenterToScene(sceneX, sceneY, x, y);

    // asks the Widget Interactor to provide a mouse tracker
    std::auto_ptr<IWorldSceneMouseTracker> tracker;

    if (interactor_)
    {
      tracker.reset(interactor_->CreateMouseTracker(*this, view_, button, modifiers, x, y, sceneX, sceneY, GetStatusBar()));
    }
    
    if (tracker.get() != NULL)
    {
      return new SceneMouseTracker(view_, tracker.release());
    }
    else if (hasDefaultMouseEvents_)
    {
      switch (button)
      {
        case MouseButton_Middle:
          return new SceneMouseTracker(view_, new PanMouseTracker(*this, x, y));

        case MouseButton_Right:
          return new SceneMouseTracker(view_, new ZoomMouseTracker(*this, x, y));

        default:
          return NULL;
      }      
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
