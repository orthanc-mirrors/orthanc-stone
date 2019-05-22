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


#include "TestCairoWidget.h"

#include <stdio.h>


namespace Deprecated
{
  namespace Samples
  {
    void TestCairoWidget::DoAnimation() 
    {
      value_ -= 0.01f;
      if (value_ < 0)
      {
        value_ = 1;
      }

      NotifyContentChanged();
    }


    bool TestCairoWidget::RenderCairo(OrthancStone::CairoContext& context)
    {
      cairo_t* cr = context.GetObject();

      cairo_set_source_rgb (cr, .3, 0, 0);
      cairo_paint(cr);

      cairo_set_source_rgb(cr, 0, 1, 0);
      cairo_rectangle(cr, width_ / 4, height_ / 4, width_ / 2, height_ / 2);
      cairo_set_line_width(cr, 1.0);
      cairo_fill(cr);

      cairo_set_source_rgb(cr, 0, 1, value_);
      cairo_rectangle(cr, width_ / 2 - 50, height_ / 2 - 50, 100, 100);
      cairo_fill(cr);

      return true;
    }


    void TestCairoWidget::RenderMouseOverCairo(OrthancStone::CairoContext& context,
                                               int x,
                                               int y)
    {
      cairo_t* cr = context.GetObject();

      cairo_set_source_rgb (cr, 1, 0, 0);
      cairo_rectangle(cr, x - 5, y - 5, 10, 10);
      cairo_set_line_width(cr, 1.0);
      cairo_stroke(cr);

      char buf[64];
      sprintf(buf, "(%d,%d)", x, y);
      UpdateStatusBar(buf);
    }


    TestCairoWidget::TestCairoWidget(const std::string& name, bool animate) :
      CairoWidget(name),
      width_(0),
      height_(0),
      value_(1),
      animate_(animate)
    {
    }


    void TestCairoWidget::SetSize(unsigned int width, 
                                  unsigned int height)
    {
      CairoWidget::SetSize(width, height);
      width_ = width;
      height_ = height;
    }
 

    IMouseTracker* TestCairoWidget::CreateMouseTracker(OrthancStone::MouseButton button,
                                                       int x,
                                                       int y,
                                                       OrthancStone::KeyboardModifiers modifiers,
                                                       const std::vector<Touch>& touches)
    {
      UpdateStatusBar("Click");
      return NULL;
    }


    void TestCairoWidget::MouseWheel(OrthancStone::MouseWheelDirection direction,
                                     int x,
                                     int y,
                                     OrthancStone::KeyboardModifiers modifiers) 
    {
      UpdateStatusBar(direction == OrthancStone::MouseWheelDirection_Down ? "Wheel down" : "Wheel up");
    }

    
    void TestCairoWidget::KeyPressed(OrthancStone::KeyboardKeys key,
                                     char keyChar,
                                     OrthancStone::KeyboardModifiers modifiers)
    {
      UpdateStatusBar("Key pressed: \"" + std::string(1, keyChar) + "\"");
    }
  }
}
