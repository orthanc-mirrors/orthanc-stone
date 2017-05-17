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


#pragma once

#include "../Enumerations.h"
#include "../Viewport/IMouseTracker.h"
#include "../Viewport/IStatusBar.h"
#include "../Viewport/IViewport.h"

namespace OrthancStone
{
  class IWidget : public boost::noncopyable
  {
  public:
    virtual ~IWidget()
    {
    }

    virtual void SetDefaultView() = 0;

    virtual void SetParent(IWidget& parent) = 0;
    
    virtual void SetViewport(IViewport& viewport) = 0;

    virtual void SetStatusBar(IStatusBar& statusBar) = 0;

    virtual void SetSize(unsigned int width, 
                         unsigned int height) = 0;
 
    virtual bool Render(Orthanc::ImageAccessor& surface) = 0;

    virtual IMouseTracker* CreateMouseTracker(MouseButton button,
                                              int x,
                                              int y,
                                              KeyboardModifiers modifiers) = 0;

    virtual void RenderMouseOver(Orthanc::ImageAccessor& target,
                                 int x,
                                 int y) = 0;

    virtual bool HasRenderMouseOver() = 0;

    virtual void MouseWheel(MouseWheelDirection direction,
                            int x,
                            int y,
                            KeyboardModifiers modifiers) = 0;

    virtual void KeyPressed(char key,
                            KeyboardModifiers modifiers) = 0;

    virtual bool HasUpdateContent() const = 0;

    virtual void UpdateContent() = 0;

    // Subclasses can call this method to signal the display of the
    // widget must be refreshed
    virtual void NotifyChange() = 0;

    virtual void Start() = 0;
  };
}
