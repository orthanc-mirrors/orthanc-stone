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

#include "ILayerRenderer.h"

namespace OrthancStone
{
  class MissingLayerRenderer : public ILayerRenderer
  {
  private:
    double       x1_;
    double       y1_;
    double       x2_;
    double       y2_;
    RenderStyle  style_;

  public:
    MissingLayerRenderer(double x1,
                         double y1,
                         double x2,
                         double y2) : 
      x1_(x1),
      y1_(y1),
      x2_(x2),
      y2_(y2)
    {
      if (x1_ > x2_)
      {
        std::swap(x1_, x2_);
      }

      if (y1_ > y2_)
      {
        std::swap(y1_, y2_);
      }
    }

    virtual bool RenderLayer(CairoContext& context,
                             const ViewportGeometry& view,
                             const SliceGeometry& viewportSlice)
    {
      if (style_.visible_)
      {
        view.ApplyTransform(context);
        context.SetSourceColor(style_.drawColor_);

        cairo_t *cr = context.GetObject();
        cairo_set_line_width(cr, 1.0 / view.GetZoom());
        cairo_rectangle(cr, x1_, y1_, x2_ - x1_, y2_ - y1_);

        double handleSize = 10.0f / view.GetZoom();
        cairo_move_to(cr, x1_ + handleSize, y1_);
        cairo_line_to(cr, x1_, y1_ + handleSize);

        cairo_stroke(cr);
      }

      return true;
    }

    virtual void SetLayerStyle(const RenderStyle& style)
    {
      style_ = style;
    }

    virtual bool IsFullQuality()
    {
      return true;
    }
  };
}
