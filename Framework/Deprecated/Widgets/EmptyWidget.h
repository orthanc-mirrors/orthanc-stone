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


#pragma once

#include "IWidget.h"

namespace Deprecated
{
  /**
   * This is a test widget that simply fills its surface with an
   * uniform color.
   **/
  class EmptyWidget : public IWidget
  {
  private:
    uint8_t  red_;
    uint8_t  green_;
    uint8_t  blue_;

  public:
    EmptyWidget(uint8_t red,
                uint8_t green,
                uint8_t blue) :
      red_(red),
      green_(green),
      blue_(blue)
    {
    }

    virtual void FitContent()
    {
    }

    virtual void SetParent(IWidget& widget)
    {
    }

    virtual void SetViewport(WidgetViewport& viewport)
    {
    }

    virtual void NotifyContentChanged()
    {
    }

    virtual void SetStatusBar(IStatusBar& statusBar)
    {
    }

    virtual void SetSize(unsigned int width,
                         unsigned int height)
    {
    }

    virtual bool Render(Orthanc::ImageAccessor& surface);

    virtual IMouseTracker* CreateMouseTracker(OrthancStone::MouseButton button,
                                              int x,
                                              int y,
                                              OrthancStone::KeyboardModifiers modifiers,
                                              const std::vector<Touch>& touches)
    {
      return NULL;
    }

    virtual void RenderMouseOver(Orthanc::ImageAccessor& target,
                                 int x,
                                 int y)
    {
    }

    virtual void MouseWheel(OrthancStone::MouseWheelDirection direction,
                            int x,
                            int y,
                            OrthancStone::KeyboardModifiers modifiers)
    {
    }

    virtual void KeyPressed(OrthancStone::KeyboardKeys key,
                            char keyChar,
                            OrthancStone::KeyboardModifiers modifiers)
    {
    }

    virtual bool HasAnimation() const
    {
      return false;
    }

    virtual void DoAnimation();

    virtual bool HasRenderMouseOver()
    {
      return false;
    }
  };
}