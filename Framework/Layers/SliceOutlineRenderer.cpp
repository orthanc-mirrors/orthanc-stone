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


#include "SliceOutlineRenderer.h"

namespace OrthancStone
{
  bool SliceOutlineRenderer::RenderLayer(CairoContext& context,
                                         const ViewportGeometry& view)
  {
    if (style_.visible_)
    {
      cairo_t *cr = context.GetObject();
      cairo_save(cr);

      context.SetSourceColor(style_.drawColor_);

      double x1 = -0.5 * pixelSpacingX_;
      double y1 = -0.5 * pixelSpacingY_;
        
      cairo_set_line_width(cr, 1.0 / view.GetZoom());
      cairo_rectangle(cr, x1, y1,
                      static_cast<double>(width_) * pixelSpacingX_,
                      static_cast<double>(height_) * pixelSpacingY_);

      double handleSize = 10.0f / view.GetZoom();
      cairo_move_to(cr, x1 + handleSize, y1);
      cairo_line_to(cr, x1, y1 + handleSize);

      cairo_stroke(cr);
      cairo_restore(cr);
    }

    return true;
  }
}
