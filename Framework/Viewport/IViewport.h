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

#include "IStatusBar.h"
#include "../StoneEnumerations.h"
#include "../Messages/IObservable.h"

#include <Core/Images/ImageAccessor.h>
#include "../Viewport/IMouseTracker.h" // only to get the "Touch" definition

namespace OrthancStone
{
  class IWidget;   // Forward declaration
  
  class IViewport : public IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, ViewportChangedMessage, IViewport);

    IViewport(MessageBroker& broker) :
      IObservable(broker)
    {
    }
    
    virtual ~IViewport()
    {
    }

    virtual void FitContent() = 0;

    virtual void SetStatusBar(IStatusBar& statusBar) = 0;

    virtual void SetSize(unsigned int width,
                         unsigned int height) = 0;

    // The function returns "true" iff. a new frame was rendered
    virtual bool Render(Orthanc::ImageAccessor& surface) = 0;

    virtual void MouseDown(MouseButton button,
                           int x,
                           int y,
                           KeyboardModifiers modifiers,
                           const std::vector<Touch>& touches) = 0;

    virtual void MouseUp() = 0;

    virtual void MouseMove(int x, 
                           int y,
                           const std::vector<Touch>& displayTouches) = 0;

    virtual void MouseEnter() = 0;

    virtual void MouseLeave() = 0;

    virtual void MouseWheel(MouseWheelDirection direction,
                            int x,
                            int y,
                            KeyboardModifiers modifiers) = 0;

    virtual void KeyPressed(KeyboardKeys key,
                            char keyChar,
                            KeyboardModifiers modifiers) = 0;

    virtual bool HasAnimation() = 0;

    virtual void DoAnimation() = 0;

    // Should only be called from IWidget
    // TODO Why should this be virtual?
    virtual void NotifyContentChanged()
    {
      BroadcastMessage(ViewportChangedMessage(*this));
    }
  };
}
