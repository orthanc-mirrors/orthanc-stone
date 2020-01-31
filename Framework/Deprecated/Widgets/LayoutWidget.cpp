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


#include "LayoutWidget.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/math/special_functions/round.hpp>

namespace Deprecated
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
      Orthanc::ImageAccessor accessor;
      surface.GetRegion(accessor, left_, top_, width_, height_);
      tracker_->Render(accessor);
    }

    virtual void MouseUp()
    {
      tracker_->MouseUp();
    }

    virtual void MouseMove(int x, 
                           int y,
                           const std::vector<Touch>& displayTouches)
    {
      std::vector<Touch> relativeTouches;
      for (size_t t = 0; t < displayTouches.size(); t++)
      {
        relativeTouches.push_back(Touch(displayTouches[t].x - left_, displayTouches[t].y - top_));
      }

      tracker_->MouseMove(x - left_, y - top_, relativeTouches);
    }
  };


  class LayoutWidget::ChildWidget : public boost::noncopyable
  {
  private:
    boost::shared_ptr<IWidget>  widget_;
    int                     left_;
    int                     top_;
    unsigned int            width_;
    unsigned int            height_;

  public:
    ChildWidget(boost::shared_ptr<IWidget> widget) :
      widget_(widget)
    {
      assert(widget != NULL);
      SetEmpty();
    }

    void DoAnimation()
    {
      if (widget_->HasAnimation())
      {
        widget_->DoAnimation();
      }
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
        Orthanc::ImageAccessor accessor;
        target.GetRegion(accessor, left_, top_, width_, height_);
        return widget_->Render(accessor);
      }
    }

    IMouseTracker* CreateMouseTracker(OrthancStone::MouseButton button,
                                      int x,
                                      int y,
                                      OrthancStone::KeyboardModifiers modifiers,
                                      const std::vector<Touch>& touches)
    {
      if (Contains(x, y))
      {
        IMouseTracker* tracker = widget_->CreateMouseTracker(button, 
                                                             x - left_, 
                                                             y - top_, 
                                                             modifiers,
                                                             touches);
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
        Orthanc::ImageAccessor accessor;
        target.GetRegion(accessor, left_, top_, width_, height_);

        widget_->RenderMouseOver(accessor, x - left_, y - top_);
      }
    }

    void MouseWheel(OrthancStone::MouseWheelDirection direction,
                    int x,
                    int y,
                    OrthancStone::KeyboardModifiers modifiers)
    {
      if (Contains(x, y))
      {
        widget_->MouseWheel(direction, x - left_, y - top_, modifiers);
      }
    }
    
    bool HasRenderMouseOver()
    {
      return widget_->HasRenderMouseOver();
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
      unsigned int padding = paddingLeft_ + paddingRight_ + (static_cast<unsigned int>(children_.size()) - 1) * paddingInternal_;
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
      unsigned int padding = paddingTop_ + paddingBottom_ + (static_cast<unsigned int>(children_.size()) - 1) * paddingInternal_;
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

    NotifyContentChanged(*this);
  }


  LayoutWidget::LayoutWidget(const std::string& name) :
    WidgetBase(name),
    isHorizontal_(true),
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
      delete children_[i];
    }
  }


  void LayoutWidget::FitContent()
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().FitContent();
    }
  }
  

  void LayoutWidget::NotifyContentChanged(const IWidget& widget)
  {
    // One of the children has changed
    WidgetBase::NotifyContentChanged();
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


  void LayoutWidget::AddWidget(boost::shared_ptr<IWidget> widget)  // Takes ownership
  {
    if (widget == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (GetStatusBar() != NULL)
    {
      widget->SetStatusBar(*GetStatusBar());
    }

    children_.push_back(new ChildWidget(widget));
    widget->SetParent(*this);

    ComputeChildrenExtents();

    if (widget->HasAnimation())
    {
      hasAnimation_ = true;
    }
  }


  void LayoutWidget::SetStatusBar(IStatusBar& statusBar)
  {
    WidgetBase::SetStatusBar(statusBar);

    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().SetStatusBar(statusBar);
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

    
  IMouseTracker* LayoutWidget::CreateMouseTracker(OrthancStone::MouseButton button,
                                                  int x,
                                                  int y,
                                                  OrthancStone::KeyboardModifiers modifiers,
                                                  const std::vector<Touch>& touches)
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      IMouseTracker* tracker = children_[i]->CreateMouseTracker(button, x, y, modifiers, touches);
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


  void LayoutWidget::MouseWheel(OrthancStone::MouseWheelDirection direction,
                                int x,
                                int y,
                                OrthancStone::KeyboardModifiers modifiers)
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->MouseWheel(direction, x, y, modifiers);
    }
  }


  void LayoutWidget::KeyPressed(OrthancStone::KeyboardKeys key,
                                char keyChar,
                                OrthancStone::KeyboardModifiers modifiers)
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      children_[i]->GetWidget().KeyPressed(key, keyChar, modifiers);
    }
  }

  
  void LayoutWidget::DoAnimation()
  {
    if (hasAnimation_)
    {
      for (size_t i = 0; i < children_.size(); i++)
      {
        children_[i]->DoAnimation();
      }
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  bool LayoutWidget::HasRenderMouseOver()
  {
    for (size_t i = 0; i < children_.size(); i++)
    {
      if (children_[i]->HasRenderMouseOver())
      {
        return true;
      }
    }

    return false;
  }
}
