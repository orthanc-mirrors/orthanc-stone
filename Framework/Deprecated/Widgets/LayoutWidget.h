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


#pragma once

#include "WidgetBase.h"

#include <vector>
#include <memory>

namespace Deprecated
{
  class LayoutWidget : public WidgetBase
  {
  private:
    class LayoutMouseTracker;
    class ChildWidget;

    std::vector<ChildWidget*>     children_;
    bool                          isHorizontal_;
    unsigned int                  width_;
    unsigned int                  height_;
    std::unique_ptr<IMouseTracker>  mouseTracker_;
    unsigned int                  paddingLeft_;
    unsigned int                  paddingTop_;
    unsigned int                  paddingRight_;
    unsigned int                  paddingBottom_;
    unsigned int                  paddingInternal_;
    bool                          hasAnimation_;

    void ComputeChildrenExtents();

  public:
    LayoutWidget(const std::string& name);

    virtual ~LayoutWidget();

    virtual void FitContent();

    virtual void NotifyContentChanged(const IWidget& widget);

    void SetHorizontal();

    void SetVertical();

    void SetPadding(unsigned int left,
                    unsigned int top,
                    unsigned int right,
                    unsigned int bottom,
                    unsigned int spacing);
    
    void SetPadding(unsigned int padding);

    unsigned int GetPaddingLeft() const
    {
      return paddingLeft_;
    }

    unsigned int GetPaddingTop() const
    {
      return paddingTop_;
    }

    unsigned int GetPaddingRight() const
    {
      return paddingRight_;
    }

    unsigned int GetPaddingBottom() const
    {
      return paddingBottom_;
    }

    unsigned int GetPaddingInternal() const
    {
      return paddingInternal_;
    }

    void AddWidget(boost::shared_ptr<IWidget> widget);

    virtual void SetStatusBar(IStatusBar& statusBar);

    virtual void SetSize(unsigned int width,
                         unsigned int height);

    virtual bool Render(Orthanc::ImageAccessor& surface);
    
    virtual IMouseTracker* CreateMouseTracker(OrthancStone::MouseButton button,
                                              int x,
                                              int y,
                                              OrthancStone::KeyboardModifiers modifiers,
                                              const std::vector<Touch>& touches);

    virtual void RenderMouseOver(Orthanc::ImageAccessor& target,
                                 int x,
                                 int y);

    virtual void MouseWheel(OrthancStone::MouseWheelDirection direction,
                            int x,
                            int y,
                            OrthancStone::KeyboardModifiers modifiers);

    virtual void KeyPressed(OrthancStone::KeyboardKeys key,
                            char keyChar,
                            OrthancStone::KeyboardModifiers modifiers);

    virtual bool HasAnimation() const
    {
      return hasAnimation_;
    }

    virtual void DoAnimation();

    virtual bool HasRenderMouseOver();
  };
}
