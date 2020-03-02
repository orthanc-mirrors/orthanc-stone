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

#include "IViewport.h"
#include "../Widgets/IWidget.h"

#include <Core/Compatibility.h>

#include <memory>

namespace Deprecated
{
  class WidgetViewport : public IViewport
  {
  private:
    std::unique_ptr<IWidget>        centralWidget_;
    IStatusBar*                   statusBar_;
    std::unique_ptr<IMouseTracker>  mouseTracker_;
    bool                          isMouseOver_;
    int                           lastMouseX_;
    int                           lastMouseY_;
    OrthancStone::CairoSurface    background_;
    bool                          backgroundChanged_;

  public:
    WidgetViewport(OrthancStone::MessageBroker& broker);

    virtual void FitContent();

    virtual void SetStatusBar(IStatusBar& statusBar);

    IWidget& SetCentralWidget(IWidget* widget);  // Takes ownership

    virtual void NotifyBackgroundChanged();

    virtual void SetSize(unsigned int width,
                         unsigned int height);

    virtual bool Render(Orthanc::ImageAccessor& surface);

    virtual void MouseDown(OrthancStone::MouseButton button,
                           int x,
                           int y,
                           OrthancStone::KeyboardModifiers modifiers,
                           const std::vector<Touch>& displayTouches);

    virtual void MouseUp();

    virtual void MouseMove(int x, 
                           int y,
                           const std::vector<Touch>& displayTouches);

    virtual void MouseEnter();

    virtual void MouseLeave();

    virtual void TouchStart(const std::vector<Touch>& touches);
    
    virtual void TouchMove(const std::vector<Touch>& touches);
    
    virtual void TouchEnd(const std::vector<Touch>& touches);

    virtual void MouseWheel(OrthancStone::MouseWheelDirection direction,
                            int x,
                            int y,
                            OrthancStone::KeyboardModifiers modifiers);

    virtual void KeyPressed(OrthancStone::KeyboardKeys key,
                            char keyChar,
                            OrthancStone::KeyboardModifiers modifiers);

    virtual bool HasAnimation();

    virtual void DoAnimation();
  };
}
