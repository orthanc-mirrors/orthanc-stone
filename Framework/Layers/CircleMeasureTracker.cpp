/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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


#define _USE_MATH_DEFINES  // To access M_PI in Visual Studio
#include <cmath>

#include "CircleMeasureTracker.h"

#include "../Viewport/CairoFont.h"

#include <stdio.h>

namespace OrthancStone
{
  CircleMeasureTracker::CircleMeasureTracker(IStatusBar* statusBar,
                                             const SliceGeometry& slice,
                                             double x, 
                                             double y,
                                             uint8_t red,
                                             uint8_t green,
                                             uint8_t blue,
                                             unsigned int fontSize) :
    statusBar_(statusBar),
    slice_(slice),
    x1_(x),
    y1_(y),
    x2_(x),
    y2_(y),
    fontSize_(fontSize)
  {
    color_[0] = red;
    color_[1] = green;
    color_[2] = blue;
  }
    

  void CircleMeasureTracker::Render(CairoContext& context,
                                    double zoom)
  {
    double x = (x1_ + x2_) / 2.0;
    double y = (y1_ + y2_) / 2.0;

    Vector tmp;
    GeometryToolbox::AssignVector(tmp, x2_ - x1_, y2_ - y1_);
    double r = boost::numeric::ublas::norm_2(tmp) / 2.0;

    context.SetSourceColor(color_[0], color_[1], color_[2]);

    cairo_t* cr = context.GetObject();
    cairo_save(cr);
    cairo_set_line_width(cr, 2.0 / zoom);
    cairo_translate(cr, x, y);
    cairo_arc(cr, 0, 0, r, 0, 2 * M_PI);
    cairo_stroke_preserve(cr);
    cairo_stroke(cr);
    cairo_restore(cr);

    if (fontSize_ != 0)
    {
      cairo_move_to(cr, x, y);
      CairoFont font("sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      font.Draw(context, FormatRadius(), static_cast<double>(fontSize_) / zoom);
    }
  }
    

  double CircleMeasureTracker::GetRadius() const  // In millimeters
  {
    Vector a = slice_.MapSliceToWorldCoordinates(x1_, y1_);
    Vector b = slice_.MapSliceToWorldCoordinates(x2_, y2_);
    return boost::numeric::ublas::norm_2(b - a) / 2.0;
  }


  std::string CircleMeasureTracker::FormatRadius() const
  {
    char buf[64];
    sprintf(buf, "%0.01f cm", GetRadius() / 10.0);
    return buf;
  }

  void CircleMeasureTracker::MouseMove(double x,
                                       double y)
  {
    x2_ = x;
    y2_ = y;

    if (statusBar_ != NULL)
    {
      statusBar_->SetMessage("Circle radius: " + FormatRadius());
    }
  }
}
