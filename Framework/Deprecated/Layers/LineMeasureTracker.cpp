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


#include "LineMeasureTracker.h"

#include <stdio.h>

namespace Deprecated
{
  LineMeasureTracker::LineMeasureTracker(IStatusBar* statusBar,
                                         const OrthancStone::CoordinateSystem3D& slice,
                                         double x, 
                                         double y,
                                         uint8_t red,
                                         uint8_t green,
                                         uint8_t blue,
                                         const Orthanc::Font& font) :
    statusBar_(statusBar),
    slice_(slice),
    x1_(x),
    y1_(y),
    x2_(x),
    y2_(y),
    font_(font)
  {
    color_[0] = red;
    color_[1] = green;
    color_[2] = blue;
  }
    

  void LineMeasureTracker::Render(OrthancStone::CairoContext& context,
                                  double zoom)
  {
    context.SetSourceColor(color_[0], color_[1], color_[2]);

    cairo_t* cr = context.GetObject();
    cairo_set_line_width(cr, 2.0 / zoom);
    cairo_move_to(cr, x1_, y1_);
    cairo_line_to(cr, x2_, y2_);
    cairo_stroke(cr);

    if (y2_ - y1_ < 0)
    {
      context.DrawText(font_, FormatLength(), x2_, y2_ - 5, OrthancStone::BitmapAnchor_BottomCenter);
    }
    else
    {
      context.DrawText(font_, FormatLength(), x2_, y2_ + 5, OrthancStone::BitmapAnchor_TopCenter);
    }
  }
    

  double LineMeasureTracker::GetLength() const  // In millimeters
  {
    OrthancStone::Vector a = slice_.MapSliceToWorldCoordinates(x1_, y1_);
    OrthancStone::Vector b = slice_.MapSliceToWorldCoordinates(x2_, y2_);
    return boost::numeric::ublas::norm_2(b - a);
  }


  std::string LineMeasureTracker::FormatLength() const
  {
    char buf[64];
    sprintf(buf, "%0.01f cm", GetLength() / 10.0);
    return buf;
  }

  void LineMeasureTracker::MouseMove(int displayX,
                                     int displayY,
                                     double x,
                                     double y,
                                     const std::vector<Touch>& displayTouches,
                                     const std::vector<Touch>& sceneTouches)
  {
    x2_ = x;
    y2_ = y;

    if (statusBar_ != NULL)
    {
      statusBar_->SetMessage("Line length: " + FormatLength());
    }
  }
}