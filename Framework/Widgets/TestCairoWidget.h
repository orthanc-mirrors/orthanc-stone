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

#include "CairoWidget.h"

namespace OrthancStone
{
  namespace Samples
  {
    class TestCairoWidget : public CairoWidget
    {
    private:
      unsigned int  width_;
      unsigned int  height_;
      float         value_;
      bool          animate_;

    protected:
      virtual bool RenderCairo(CairoContext& context);

      virtual void RenderMouseOverCairo(CairoContext& context,
                                        int x,
                                        int y);

    public:
      TestCairoWidget(const std::string& name, bool animate);

      virtual void SetSize(unsigned int width, 
                           unsigned int height);
 
      virtual IMouseTracker* CreateMouseTracker(MouseButton button,
                                                int x,
                                                int y,
                                                KeyboardModifiers modifiers);

      virtual void MouseWheel(MouseWheelDirection direction,
                              int x,
                              int y,
                              KeyboardModifiers modifiers);
    
      virtual void KeyPressed(KeyboardKeys key,
                              char keyChar,
                              KeyboardModifiers modifiers);

      virtual bool HasAnimation() const
      {
        return animate_;
      }
      
      virtual void DoAnimation();

      virtual bool HasRenderMouseOver()
      {
        return true;
      }
    };
  }
}
