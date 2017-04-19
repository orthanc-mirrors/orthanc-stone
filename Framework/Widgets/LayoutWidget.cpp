/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "LayoutWidget.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

#include <boost/math/special_functions/round.hpp>

namespace OrthancStone
{
  class LayoutWidget::LayoutMouseTracker : public IMouseTracker
  {
  private:
    std::auto_ptr<IMouseTracker>   tracker_;
    int                            left_;
    int                            top_;
    unsigned int                   width_;
    unsigned int                   height_;

  public:
    LayoutMouseTracker(IMouseTracker* tracker,
                       int left,
                       int top,
                       unsigned int width,
                       unsigned int height) :
      tracker_(tracker),
      left_(left),
      top_(top),
      width_(width),
      height_(height)
    {
      if (tracker == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    virtual void Render(Orthanc::ImageAccessor& surface)
    {
      Orthanc::ImageAccessor accessor = surface.GetRegion(left_, top_, width_, height_);
      tracker_->Render(accessor);
    }

    virtual void MouseUp()
    {
      tracker_->MouseUp();
    }

    virtual void MouseMove(int x, 
                           int y)
    {
      tracker_->MouseMove(x - left_, y - top_);
    }
  };


  class LayoutWidget::ChildWidget : public boost::noncopyable
  {
  private:
    std::auto_ptr<IWidget>  widget_;
    int                     left_;
    int                     top_;
    unsigned int            width_;
    unsigned int            height_;

  public:
    ChildWidget(IWidget* widget) :
      widget_(widget)
    {
      assert(widget != NULL);
      SetEmpty();
    }

    IWidget& GetWidget() const
    {
      return *widget_;
    }

    void SetRectangle(unsigned int left, 
                      unsigned int top,
                      unsigned int width,
                      unsigned int height)
    {
      left_ = left;
      top_ = top;
      width_ = width;
      height_ = height;

      widget_->SetSize(width, height);
    }

    void SetEmpty()
    {
      SetRectangle(0, 0, 0, 0);
    }

    bool Contains(int x, 
                  int y) const
    {
      return (x >= left_ && 
              y >= top_ &&
              x < left_ + static_cast<int>(width_) &&
              y < top_ + static_cast<int>(height_));
    }      

    bool Render(Orthanc::ImageAccessor& target)
    {
      if (width_ == 0 ||
          height_ == 0)
      {
        return true;
      }
      else 
      {
        Orthanc::ImageAccessor accessor = target.GetRegion(left_, top_, width_, height_);
        return widget_->Render(accessor);
      }
    }

    IMouseTracker* CreateMouseTracker(MouseButton button,
                                      int x,
                                      int y,
                                      KeyboardModifiers modifiers)
    {
      if (Contains(x, y))
      {
        IMouseTracker* tracker = widget_->CreateMouseTracker(button, 
                                                             x - left_, 
                                                             y - top_, 
                                                             modifiers);
        if (tracker)
        {
          return new LayoutMouseTracker(tracker, left_, top_, width_, height_);
        }
      }

      return NULL;
    }

    void RenderMouseOver(Orthanc::ImageAccessor& target,
                         int x,
                         int y)
    {
      if (Contains(x, y))
      {
        Orthanc::ImageAccessor accessor = target.GetRegion(left_, top_, width_, height_);

        widget_->RenderMouseOver(accessor, x - left_, y - top_);
      }
    }

    void MouseWheel(MouseWheelDirection direction,
                    int x,
                    int y,
                    KeyboardModifiers modifiers)
    {
      if (Contains(x, y))
      {
        widget_->MouseWheel(direction, x - left_, y - top_, modifiers);
      }
    }
  };


  void LayoutWidget::ComputeChildrenExtents()
  {
    if (children_.size() == 0)
    {
      return;
    }

    float internal = static_cast<float>(paddingInternal_);

    if (width_ <= paddingLeft_ + paddingRight_ ||
        height_ <= paddingTop_ + paddingBottom_)
    {
      for (size_t i = 0; i < children_.size(); i++)
      {
        children_[i]->SetEmpty();          
      }
    }
    else if (isHorizontal_)
    {
      unsigned int padding = paddingLeft_ + paddingRight_ + (children_.size() - 1) * paddingInternal_;
      float childWidth = ((static_cast<float>(width_) - static_cast<float>(padding)) / 
                          static_cast<float>(children_.size()));
        
      for (size_t i = 0; i < children_.size(); i++)
      {
        float left = static_cast<float>(paddingLeft_) + static_cast<float>(i) * (childWidth + internal);
        float right = left + childWidth;

        if (left >= right)
        {
          children_[i]->SetEmpty();
        }
        else
        {
          children_[i]->SetRectangle(static_cast<unsigned int>(left), 
                                     paddingTop_, 
                                     boost::math::iround(right - left),
                                     height_ - paddingTop_ - paddingBottom_);
        }
      }
    }
    else
    {
      unsigned int padding = paddingTop_ + paddingBottom_ + (children_.size() - 1) * paddingInternal_;
      float childHeight = ((static_cast<float>(height_) - static_cast<float>(padding)) / 
                           static_cast<float>(children_.size()));
        
      for (size_t i = 0; i < children_.size(); i++)
      {
        float top = static_cast<float>(paddingTop_) + static_cast<float>(i) * (childHeight + internal);
        float bottom = top + childHeight;

        if (top >= bottom)
        {
          children_[i]->SetEmpty();
        }
        else
        {
          children_[i]->SetRectangle(paddingTop_, 
                                     static_cast<unsigned int>(top), 
                                     width_ - paddingLeft_ - paddingRight_,
                                     boost::math::iround(bottom - top));
        }
      }
    }

    NotifyChange(*this);
  }


  LayoutWidget::LayoutWidget() :
    isHorizontal_(true),
    started_(false),
    width_(0),
    height_(0),
    paddingLeft_(0),
    paddingTop_(0),
    paddingRight_(0),
    paddingBottom_(0),
    paddingInternal_(0)
  {
  }


  LayoutWidget::~LayoutWidget()
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().Unregister(*this);
      delete children_[i];
    }
  }


  void LayoutWidget::NotifyChange(const IWidget& widget)
  {
    // One of the children has changed
    WidgetBase::NotifyChange();
  }


  void LayoutWidget::SetHorizontal()
  {
    isHorizontal_ = true;
    ComputeChildrenExtents();
  }


  void LayoutWidget::SetVertical()
  {
    isHorizontal_ = false;
    ComputeChildrenExtents();
  }


  void LayoutWidget::SetPadding(unsigned int left,
                                unsigned int top,
                                unsigned int right,
                                unsigned int bottom,
                                unsigned int spacing)
  {
    paddingLeft_ = left;
    paddingTop_ = top;
    paddingRight_ = right;
    paddingBottom_ = bottom;
    paddingInternal_ = spacing;
  }
    

  void LayoutWidget::SetPadding(unsigned int padding)
  {
    paddingLeft_ = padding;
    paddingTop_ = padding;
    paddingRight_ = padding;
    paddingBottom_ = padding;
    paddingInternal_ = padding;
  }


  IWidget& LayoutWidget::AddWidget(IWidget* widget)  // Takes ownership
  {
    if (started_)
    {
      LOG(ERROR) << "Cannot add child once Start() has been invoked";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (widget == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (GetStatusBar() != NULL)
    {
      widget->SetStatusBar(*GetStatusBar());
    }
    else
    {
      widget->ResetStatusBar();
    }

    children_.push_back(new ChildWidget(widget));
    widget->Register(*this);

    ComputeChildrenExtents();

    return *widget;
  }


  void LayoutWidget::SetStatusBar(IStatusBar& statusBar)
  {
    WidgetBase::SetStatusBar(statusBar);

    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().SetStatusBar(statusBar);
    }
  }


  void LayoutWidget::ResetStatusBar()
  {
    WidgetBase::ResetStatusBar();

    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().ResetStatusBar();
    }
  }  


  void LayoutWidget::Start()
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().Start();
    }

    WidgetBase::Start();
  }


  void LayoutWidget::Stop()
  {
    WidgetBase::Stop();

    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().Stop();
    }
  }


  void LayoutWidget::SetSize(unsigned int width,
                             unsigned int height)
  {
    width_ = width;
    height_ = height;
    ComputeChildrenExtents();
  }


  bool LayoutWidget::Render(Orthanc::ImageAccessor& surface)
  {
    if (!WidgetBase::Render(surface))
    {
      return false;
    }

    for (size_t i = 0; i < children_.size(); i++)
    {
      if (!children_[i]->Render(surface))
      {
        return false;
      }
    }

    return true;
  }

    
  IMouseTracker* LayoutWidget::CreateMouseTracker(MouseButton button,
                                                  int x,
                                                  int y,
                                                  KeyboardModifiers modifiers)
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      IMouseTracker* tracker = children_[i]->CreateMouseTracker(button, x, y, modifiers);
      if (tracker != NULL)
      {
        return tracker;
      }
    }

    return NULL;
  }


  void LayoutWidget::RenderMouseOver(Orthanc::ImageAccessor& target,
                                     int x,
                                     int y)
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->RenderMouseOver(target, x, y);
    }
  }


  void LayoutWidget::MouseWheel(MouseWheelDirection direction,
                                int x,
                                int y,
                                KeyboardModifiers modifiers)
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->MouseWheel(direction, x, y, modifiers);
    }
  }


  void LayoutWidget::KeyPressed(char key,
                                KeyboardModifiers modifiers)
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().KeyPressed(key, modifiers);
    }
  }
}
