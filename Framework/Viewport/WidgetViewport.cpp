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


#include "WidgetViewport.h"

#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  void WidgetViewport::UnregisterCentralWidget()
  {
    mouseTracker_.reset(NULL);

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->Unregister(*this);
    }
  }


  WidgetViewport::WidgetViewport() :
    statusBar_(NULL),
    isMouseOver_(false),
    lastMouseX_(0),
    lastMouseY_(0),
    backgroundChanged_(false),
    started_(false)
  {
  }


  void WidgetViewport::SetStatusBar(IStatusBar& statusBar)
  {
    statusBar_ = &statusBar;

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->SetStatusBar(statusBar);
    }
  }


  void WidgetViewport::ResetStatusBar()
  {
    statusBar_ = NULL;

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->ResetStatusBar();
    }
  }


  IWidget& WidgetViewport::SetCentralWidget(IWidget* widget)
  {
    if (started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (widget == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    UnregisterCentralWidget();
      
    centralWidget_.reset(widget);
    centralWidget_->Register(*this);

    if (statusBar_ == NULL)
    {
      centralWidget_->ResetStatusBar();
    }
    else
    {
      centralWidget_->SetStatusBar(*statusBar_);
    }

    backgroundChanged_ = true;

    return *widget;
  }


  void WidgetViewport::NotifyChange(const IWidget& widget)
  {
    backgroundChanged_ = true;
    observers_.NotifyChange(this);
  }


  void WidgetViewport::Start()
  {
    if (centralWidget_.get() != NULL)
    {
      centralWidget_->Start();
    }

    started_ = true;
  }


  void WidgetViewport::Stop()
  {
    started_ = false;

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->Stop();
    }
  }


  void WidgetViewport::SetSize(unsigned int width,
                               unsigned int height)
  {
    background_.SetSize(width, height);

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->SetSize(width, height);
    }

    observers_.NotifyChange(this);
  }


  bool WidgetViewport::Render(Orthanc::ImageAccessor& surface)
  {
    if (!started_ ||
        centralWidget_.get() == NULL)
    {
      return false;
    }
    
    Orthanc::ImageAccessor background = background_.GetAccessor();

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
    if (!started_)
    {
      return;
    }

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

    observers_.NotifyChange(this);;
  }


  void WidgetViewport::MouseUp()
  {
    if (!started_)
    {
      return;
    }

    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseUp();
      mouseTracker_.reset(NULL);
      observers_.NotifyChange(this);;
    }
  }


  void WidgetViewport::MouseMove(int x, 
                                 int y) 
  {
    if (!started_)
    {
      return;
    }

    lastMouseX_ = x;
    lastMouseY_ = y;

    if (mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseMove(x, y);
    }

    // The scene must be repainted
    observers_.NotifyChange(this);
  }


  void WidgetViewport::MouseEnter()
  {
    isMouseOver_ = true;
    observers_.NotifyChange(this);
  }


  void WidgetViewport::MouseLeave()
  {
    isMouseOver_ = false;

    if (started_ &&
        mouseTracker_.get() != NULL)
    {
      mouseTracker_->MouseUp();
      mouseTracker_.reset(NULL);
    }

    observers_.NotifyChange(this);
  }


  void WidgetViewport::MouseWheel(MouseWheelDirection direction,
                                  int x,
                                  int y,
                                  KeyboardModifiers modifiers)
  {
    if (!started_)
    {
      return;
    }

    if (centralWidget_.get() != NULL &&
        mouseTracker_.get() == NULL)
    {
      centralWidget_->MouseWheel(direction, x, y, modifiers);
    }
  }


  void WidgetViewport::KeyPressed(char key,
                                  KeyboardModifiers modifiers)
  {
    if (centralWidget_.get() != NULL &&
        mouseTracker_.get() == NULL)
    {
      centralWidget_->KeyPressed(key, modifiers);
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
