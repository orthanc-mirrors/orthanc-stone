/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
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


#pragma once

#include "IWidget.h"

namespace OrthancStone
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

    virtual void SetStatusBar(IStatusBar& statusBar)
    {
    }

    virtual void ResetStatusBar()
    {
    }

    virtual void Register(IChangeObserver& observer)
    {
    }

    virtual void Unregister(IChangeObserver& observer)
    {
    }

    virtual void Start()
    {
    }

    virtual void Stop()
    {
    }

    virtual void SetSize(unsigned int width, 
                         unsigned int height)
    {
    }
 
    virtual bool Render(Orthanc::ImageAccessor& surface);

    virtual IMouseTracker* CreateMouseTracker(MouseButton button,
                                              int x,
                                              int y,
                                              KeyboardModifiers modifiers)
    {
      return NULL;
    }

    virtual void RenderMouseOver(Orthanc::ImageAccessor& target,
                                 int x,
                                 int y)
    {
    }

    virtual void MouseWheel(MouseWheelDirection direction,
                            int x,
                            int y,
                            KeyboardModifiers modifiers)
    {
    }

    virtual void KeyPressed(char key,
                            KeyboardModifiers modifiers)
    {
    }
  };
}
