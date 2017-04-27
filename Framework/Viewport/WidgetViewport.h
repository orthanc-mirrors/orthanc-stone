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

#include "IViewport.h"
#include "../Toolbox/ObserversRegistry.h"
#include "../Widgets/IWidget.h"

#include <memory>

namespace OrthancStone
{
  class WidgetViewport : 
    public IViewport,
    public IWidget::IChangeObserver    
  {
  private:
    std::auto_ptr<IWidget>        centralWidget_;
    IStatusBar*                   statusBar_;
    ObserversRegistry<IViewport>  observers_;
    std::auto_ptr<IMouseTracker>  mouseTracker_;
    bool                          isMouseOver_;
    int                           lastMouseX_;
    int                           lastMouseY_;
    CairoSurface                  background_;
    bool                          backgroundChanged_;
    bool                          started_;

    void UnregisterCentralWidget();

  public:
    WidgetViewport();

    virtual ~WidgetViewport()
    {
      UnregisterCentralWidget();
    }

    virtual void SetStatusBar(IStatusBar& statusBar);

    virtual void ResetStatusBar();

    IWidget& SetCentralWidget(IWidget* widget);  // Takes ownership

    virtual void NotifyChange(const IWidget& widget);

    virtual void Register(IViewport::IChangeObserver& observer)
    {
      observers_.Register(observer);
    }

    virtual void Unregister(IViewport::IChangeObserver& observer)
    {
      observers_.Unregister(observer);
    }

    virtual void Start();

    virtual void Stop();

    virtual void SetSize(unsigned int width,
                         unsigned int height);

    virtual bool Render(Orthanc::ImageAccessor& surface);

    virtual void MouseDown(MouseButton button,
                           int x,
                           int y,
                           KeyboardModifiers modifiers);

    virtual void MouseUp();

    virtual void MouseMove(int x, 
                           int y);

    virtual void MouseEnter();

    virtual void MouseLeave();

    virtual void MouseWheel(MouseWheelDirection direction,
                            int x,
                            int y,
                            KeyboardModifiers modifiers);

    virtual void KeyPressed(char key,
                            KeyboardModifiers modifiers);

    virtual bool HasUpdateContent();

    virtual void UpdateContent();
  };
}
