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


#include "WidgetViewport.h"

#include <Core/Images/ImageProcessing.h>
#include <Core/OrthancException.h>

namespace Deprecated
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


  void WidgetViewport::SetCentralWidget(boost::shared_ptr<IWidget> widget)
  {
    if (widget == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    mouseTracker_.reset(NULL);

    centralWidget_ = widget;
    centralWidget_->SetViewport(*this);

    if (statusBar_ != NULL)
    {
      centralWidget_->SetStatusBar(*statusBar_);
    }

    NotifyBackgroundChanged();
  }


  void WidgetViewport::NotifyBackgroundChanged()
  {
    backgroundChanged_ = true;
    NotifyContentChanged();
  }


  void WidgetViewport::SetSize(unsigned int width,
                               unsigned int height)
  {
    background_.SetSize(width, height, false /* no alpha */);

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->SetSize(width, height);
    }

    NotifyBackgroundChanged();
  }


  bool WidgetViewport::Render(Orthanc::ImageAccessor& surface)
  {
    if (centralWidget_.get() == NULL)
    {
      return false;
    }

    Orthanc::ImageAccessor background;
    background_.GetWriteableAccessor(background);

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

  void WidgetViewport::TouchStart(const std::vector<Touch>& displayTouches)
  {
    MouseDown(OrthancStone::MouseButton_Left, (int)displayTouches[0].x, (int)displayTouches[0].y, OrthancStone::KeyboardModifiers_None, displayTouches); // one touch is equivalent to a mouse tracker without left button -> set the mouse coordinates to the first touch coordinates
  }
      
  void WidgetViewport::TouchMove(const std::vector<Touch>& displayTouches)
  {
    MouseMove((int)displayTouches[0].x, (int)displayTouches[0].y, displayTouches); // one touch is equivalent to a mouse tracker without left button -> set the mouse coordinates to the first touch coordinates
  }
      
  void WidgetViewport::TouchEnd(const std::vector<Touch>& displayTouches)
  {
    // note: TouchEnd is not triggered when a single touch gesture ends (it is only triggered when
    // going from 2 touches to 1 touch, ...)
    MouseUp();
  }

  void WidgetViewport::MouseDown(OrthancStone::MouseButton button,
                                 int x,
                                 int y,
                                 OrthancStone::KeyboardModifiers modifiers,
                                 const std::vector<Touch>& displayTouches
                                 )
  {
    lastMouseX_ = x;
    lastMouseY_ = y;

    if (centralWidget_.get() != NULL)
    {
      mouseTracker_.reset(centralWidget_->CreateMouseTracker(button, x, y, modifiers, displayTouches));
    }
    else
    {
      mouseTracker_.reset(NULL);
    }      

    NotifyContentChanged();
  }


  void WidgetViewport::MouseUp()
  {
    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseUp();
      mouseTracker_.reset(NULL);
      NotifyContentChanged();
    }
  }


  void WidgetViewport::MouseMove(int x, 
                                 int y,
                                 const std::vector<Touch>& displayTouches)
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
      mouseTracker_->MouseMove(x, y, displayTouches);
      repaint = true;
    }
    else
    {
      repaint = centralWidget_->HasRenderMouseOver();
    }

    if (repaint)
    {
      // The scene must be repainted, notify the observers
      NotifyContentChanged();
    }
  }


  void WidgetViewport::MouseEnter()
  {
    isMouseOver_ = true;
    NotifyContentChanged();
  }


  void WidgetViewport::MouseLeave()
  {
    isMouseOver_ = false;

    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseUp();
      mouseTracker_.reset(NULL);
    }

    NotifyContentChanged();
  }


  void WidgetViewport::MouseWheel(OrthancStone::MouseWheelDirection direction,
                                  int x,
                                  int y,
                                  OrthancStone::KeyboardModifiers modifiers)
  {
    if (centralWidget_.get() != NULL &&
        mouseTracker_.get() == NULL)
    {
      centralWidget_->MouseWheel(direction, x, y, modifiers);
    }
  }


  void WidgetViewport::KeyPressed(OrthancStone::KeyboardKeys key,
                                  char keyChar,
                                  OrthancStone::KeyboardModifiers modifiers)
  {
    if (centralWidget_.get() != NULL &&
        mouseTracker_.get() == NULL)
    {
      centralWidget_->KeyPressed(key, keyChar, modifiers);
    }
  }


  bool WidgetViewport::HasAnimation()
  {
    if (centralWidget_.get() != NULL)
    {
      return centralWidget_->HasAnimation();
    }
    else
    {
      return false;
    }
  }
   

  void WidgetViewport::DoAnimation()
  {
    if (centralWidget_.get() != NULL)
    {
      centralWidget_->DoAnimation();
    }
  }
}
