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
    boost::mutex::scoped_lock lock(mutex_);

    statusBar_ = &statusBar;

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->SetStatusBar(statusBar);
    }
  }


  void WidgetViewport::ResetStatusBar()
  {
    boost::mutex::scoped_lock lock(mutex_);

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

    boost::mutex::scoped_lock lock(mutex_);

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
    boost::mutex::scoped_lock lock(mutex_);

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->Start();
    }

    started_ = true;
  }


  void WidgetViewport::Stop()
  {
    boost::mutex::scoped_lock lock(mutex_);

    started_ = false;

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->Stop();
    }
  }


  void WidgetViewport::SetSize(unsigned int width,
                               unsigned int height)
  {
    boost::mutex::scoped_lock lock(mutex_);

    background_.SetSize(width, height);

    if (centralWidget_.get() != NULL)
    {
      centralWidget_->SetSize(width, height);
    }

    observers_.NotifyChange(this);
  }


  bool WidgetViewport::Render(Orthanc::ImageAccessor& surface)
  {
    boost::mutex::scoped_lock lock(mutex_);

    if (!started_ ||
        centralWidget_.get() == NULL)
    {
      return false;
    }

    if (backgroundChanged_)
    {
      Orthanc::ImageAccessor accessor = background_.GetAccessor();
      if (!centralWidget_->Render(accessor))
      {
        return false;
      }
    }

    Orthanc::ImageProcessing::Copy(surface, background_.GetAccessor());

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
    boost::mutex::scoped_lock lock(mutex_);

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
    boost::mutex::scoped_lock lock(mutex_);

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
    boost::mutex::scoped_lock lock(mutex_);

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
    boost::mutex::scoped_lock lock(mutex_);
    isMouseOver_ = true;
    observers_.NotifyChange(this);
  }


  void WidgetViewport::MouseLeave()
  {
    boost::mutex::scoped_lock lock(mutex_);
    isMouseOver_ = false;
    observers_.NotifyChange(this);
  }


  void WidgetViewport::MouseWheel(MouseWheelDirection direction,
                                  int x,
                                  int y,
                                  KeyboardModifiers modifiers)
  {
    boost::mutex::scoped_lock lock(mutex_);

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
    boost::mutex::scoped_lock lock(mutex_);

    if (centralWidget_.get() != NULL &&
        mouseTracker_.get() == NULL)
    {
      centralWidget_->KeyPressed(key, modifiers);
    }
  }
}
