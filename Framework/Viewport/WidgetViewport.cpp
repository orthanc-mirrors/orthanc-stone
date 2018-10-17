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


#include "WidgetViewport.h"

#include <Core/Images/ImageProcessing.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  WidgetViewport::WidgetViewport() :
    statusBar_(NULL),
    isMouseOver_(false),
    lastMouseX_(0),
    lastMouseY_(0),
    backgroundChanged_(false)
  {
  }


  void WidgetViewport::FitContent()
  {
    if (centralWidget_.get() != NULL)
    {
      centralWidget_->FitContent();
    }
  }


  void WidgetViewport::SetStatusBar(IStatusBar& statusBar)
  {
    statusBar_ = &statusBar;

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->SetStatusBar(statusBar);
    }
  }


  IWidget& WidgetViewport::SetCentralWidget(IWidget* widget)
  {
    if (widget == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    mouseTracker_.reset(NULL);

    centralWidget_.reset(widget);
    centralWidget_->SetViewport(*this);

    if (statusBar_ != NULL)
    {
      centralWidget_->SetStatusBar(*statusBar_);
    }

    backgroundChanged_ = true;
    observers_.Apply(*this, &IObserver::OnViewportContentChanged);

    return *widget;
  }


  void WidgetViewport::NotifyContentChanged(const IWidget& widget)
  {
    backgroundChanged_ = true;
    observers_.Apply(*this, &IObserver::OnViewportContentChanged);
  }


  void WidgetViewport::SetSize(unsigned int width,
                               unsigned int height)
  {
    background_.SetSize(width, height);

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->SetSize(width, height);
    }

    observers_.Apply(*this, &IObserver::OnViewportContentChanged);
  }


  bool WidgetViewport::Render(Orthanc::ImageAccessor& surface)
  {
    if (centralWidget_.get() == NULL)
    {
      return false;
    }
    
    Orthanc::ImageAccessor background;
    background_.GetAccessor(background);

    if (backgroundChanged_ &&
        !centralWidget_->Render(background))
    {
      return false;
    }

    if (background.GetWidth() != surface.GetWidth() ||
        background.GetHeight() != surface.GetHeight())
    {
      return false;
    }

    Orthanc::ImageProcessing::Convert(surface, background);
    
    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->Render(surface);
    }
    else if (isMouseOver_)
    {
      centralWidget_->RenderMouseOver(surface, lastMouseX_, lastMouseY_);
    }

    return true;
  }


  void WidgetViewport::MouseDown(MouseButton button,
                                 int x,
                                 int y,
                                 KeyboardModifiers modifiers)
  {
    lastMouseX_ = x;
    lastMouseY_ = y;

    if (centralWidget_.get() != NULL)
    {
      mouseTracker_.reset(centralWidget_->CreateMouseTracker(button, x, y, modifiers));
    }
    else
    {
      mouseTracker_.reset(NULL);
    }      

    observers_.Apply(*this, &IObserver::OnViewportContentChanged);
  }


  void WidgetViewport::MouseUp()
  {
    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseUp();
      mouseTracker_.reset(NULL);
      observers_.Apply(*this, &IObserver::OnViewportContentChanged);
    }
  }


  void WidgetViewport::MouseMove(int x, 
                                 int y) 
  {
    if (centralWidget_.get() == NULL)
    {
      return;
    }

    lastMouseX_ = x;
    lastMouseY_ = y;

    bool repaint = false;
    
    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseMove(x, y);
      repaint = true;
    }
    else
    {
      repaint = centralWidget_->HasRenderMouseOver();
    }

    if (repaint)
    {
      // The scene must be repainted, notify the observers
      observers_.Apply(*this, &IObserver::OnViewportContentChanged);
    }
  }


  void WidgetViewport::MouseEnter()
  {
    isMouseOver_ = true;
    observers_.Apply(*this, &IObserver::OnViewportContentChanged);
  }


  void WidgetViewport::MouseLeave()
  {
    isMouseOver_ = false;

    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseUp();
      mouseTracker_.reset(NULL);
    }

    observers_.Apply(*this, &IObserver::OnViewportContentChanged);
  }


  void WidgetViewport::MouseWheel(MouseWheelDirection direction,
                                  int x,
                                  int y,
                                  KeyboardModifiers modifiers)
  {
    if (centralWidget_.get() != NULL &&
        mouseTracker_.get() == NULL)
    {
      centralWidget_->MouseWheel(direction, x, y, modifiers);
    }
  }


  void WidgetViewport::KeyPressed(KeyboardKeys key,
                                  char keyChar,
                                  KeyboardModifiers modifiers)
  {
    if (centralWidget_.get() != NULL &&
        mouseTracker_.get() == NULL)
    {
      centralWidget_->KeyPressed(key, keyChar, modifiers);
    }
  }


  bool WidgetViewport::HasUpdateContent()
  {
    if (centralWidget_.get() != NULL)
    {
      return centralWidget_->HasUpdateContent();
    }
    else
    {
      return false;
    }
  }
   

  void WidgetViewport::UpdateContent()
  {
    if (centralWidget_.get() != NULL)
    {
      centralWidget_->UpdateContent();
    }
  }
}
