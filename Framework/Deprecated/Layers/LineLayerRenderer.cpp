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


#include "LineLayerRenderer.h"

namespace Deprecated
{
  LineLayerRenderer::LineLayerRenderer(double x1,
                                       double y1,
                                       double x2,
                                       double y2,
                                       const OrthancStone::CoordinateSystem3D& plane) : 
    x1_(x1),
    y1_(y1),
    x2_(x2),
    y2_(y2),
    plane_(plane)
  {
    RenderStyle style;
    SetLayerStyle(style);
  }


  bool LineLayerRenderer::RenderLayer(OrthancStone::CairoContext& context,
                                      const ViewportGeometry& view)
  {
    if (visible_)
    {
      context.SetSourceColor(color_);

      cairo_t *cr = context.GetObject();
      cairo_set_line_width(cr, 1.0 / view.GetZoom());
      cairo_move_to(cr, x1_, y1_);
      cairo_line_to(cr, x2_, y2_);
      cairo_stroke(cr);
    }

    return true;
  }


  void LineLayerRenderer::SetLayerStyle(const RenderStyle& style)
  {
    visible_ = style.visible_;
    color_[0] = style.drawColor_[0];
    color_[1] = style.drawColor_[1];
    color_[2] = style.drawColor_[2];
  }
}
