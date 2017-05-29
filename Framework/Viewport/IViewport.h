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

#include "IStatusBar.h"
#include "../Enumerations.h"

#include "../../Resources/Orthanc/Core/Images/ImageAccessor.h"

namespace OrthancStone
{
  class IWidget;   // Forward declaration
  
  class IViewport : public boost::noncopyable
  {
  public:
    class IObserver : public boost::noncopyable
    {
    public:
      virtual ~IObserver()
      {
      }

      virtual void NotifyChange(const IViewport& scene) = 0;
    };

    virtual ~IViewport()
    {
    }

    virtual void SetDefaultView() = 0;

    virtual void Register(IObserver& observer) = 0;

    virtual void SetStatusBar(IStatusBar& statusBar) = 0;

    virtual void SetSize(unsigned int width,
                         unsigned int height) = 0;

    // The function returns "true" iff. a new frame was rendered
    virtual bool Render(Orthanc::ImageAccessor& surface) = 0;

    virtual void MouseDown(MouseButton button,
                           int x,
                           int y,
                           KeyboardModifiers modifiers) = 0;

    virtual void MouseUp() = 0;

    virtual void MouseMove(int x, 
                           int y) = 0;

    virtual void MouseEnter() = 0;

    virtual void MouseLeave() = 0;

    virtual void MouseWheel(MouseWheelDirection direction,
                            int x,
                            int y,
                            KeyboardModifiers modifiers) = 0;

    virtual void KeyPressed(char key,
                            KeyboardModifiers modifiers) = 0;

    virtual bool HasUpdateContent() = 0;

    virtual void UpdateContent() = 0;

    // Should only be called from IWidget
    virtual void NotifyChange(const IWidget& widget) = 0;
  };
}
