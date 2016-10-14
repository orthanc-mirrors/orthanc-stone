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


#include "TestCairoWidget.h"

#include "../Orthanc/Core/Toolbox.h"

#include <stdio.h>


namespace OrthancStone
{
  namespace Samples
  {
    void TestCairoWidget::UpdateStep() 
    {
      value_ -= 0.01f;
      if (value_ < 0)
      {
        value_ = 1;
      }

      NotifyChange();

      Orthanc::Toolbox::USleep(25000);
    }


    bool TestCairoWidget::RenderCairo(CairoContext& context)
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


    void TestCairoWidget::RenderMouseOverCairo(CairoContext& context,
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


    TestCairoWidget::TestCairoWidget(bool animate) :
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
 

    IMouseTracker* TestCairoWidget::CreateMouseTracker(MouseButton button,
                                                       int x,
                                                       int y,
                                                       KeyboardModifiers modifiers)
    {
      UpdateStatusBar("Click");
      return NULL;
    }


    void TestCairoWidget::MouseWheel(MouseWheelDirection direction,
                                     int x,
                                     int y,
                                     KeyboardModifiers modifiers) 
    {
      UpdateStatusBar(direction == MouseWheelDirection_Down ? "Wheel down" : "Wheel up");
    }

    
    void TestCairoWidget::KeyPressed(char key,
                                     KeyboardModifiers modifiers)
    {
      UpdateStatusBar("Key pressed: \"" + std::string(1, key) + "\"");
    }
  }
}
