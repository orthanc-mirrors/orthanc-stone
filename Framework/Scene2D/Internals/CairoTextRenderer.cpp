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


#include "CairoTextRenderer.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  namespace Internals
  {
    CairoTextRenderer::CairoTextRenderer(ICairoContextProvider& target,
                                         const GlyphBitmapAlphabet& alphabet,
                                         const TextSceneLayer& layer) :
      CairoBaseRenderer(target, layer)
    {
      std::auto_ptr<Orthanc::ImageAccessor> source(alphabet.RenderText(layer.GetText()));

      if (source.get() != NULL)
      {
        text_.SetSize(source->GetWidth(), source->GetHeight(), true);

        Orthanc::ImageAccessor target;
        text_.GetWriteableAccessor(target);
        
        if (source->GetFormat() != Orthanc::PixelFormat_Grayscale8 ||
            target.GetFormat() != Orthanc::PixelFormat_BGRA32)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
        
        const unsigned int width = source->GetWidth();
        const unsigned int red = layer.GetRed();
        const unsigned int green = layer.GetGreen();
        const unsigned int blue = layer.GetBlue();

        for (unsigned int y = 0; y < source->GetHeight(); y++)
        {
          const uint8_t* p = reinterpret_cast<const uint8_t*>(source->GetConstRow(y));
          uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));
          
          for (unsigned int x = 0; x < width; x++)
          {
            unsigned int alpha = *p;

            // Premultiplied alpha
            q[0] = static_cast<uint8_t>((blue * alpha) / 255);
            q[1] = static_cast<uint8_t>((green * alpha) / 255);
            q[2] = static_cast<uint8_t>((red * alpha) / 255);
            q[3] = *p;
            
            p++;
            q += 4;
          }
        }

        cairo_surface_mark_dirty(text_.GetObject());
      }
    }

      
    void CairoTextRenderer::Render(const AffineTransform2D& transform)
    {
      if (text_.GetWidth() != 0 &&
          text_.GetHeight() != 0)
      {
        const TextSceneLayer& layer = GetLayer<TextSceneLayer>();
      
        cairo_t* cr = GetCairoContext();
        cairo_set_source_rgb(cr, layer.GetRedAsFloat(), layer.GetGreenAsFloat(), layer.GetBlueAsFloat());

        double dx, dy;  // In pixels
        ComputeAnchorTranslation(dx, dy, layer.GetAnchor(), text_.GetWidth(),
                                 text_.GetHeight(), layer.GetBorder());
      
        double x = layer.GetX();
        double y = layer.GetY();
        transform.Apply(x, y);

        cairo_save(cr);

        cairo_matrix_t t;
        cairo_matrix_init_identity(&t);
        cairo_matrix_translate(&t, x + dx, y + dy);
        cairo_transform(cr, &t);

        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_set_source_surface(cr, text_.GetObject(), 0, 0);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
        cairo_paint(cr);

        cairo_restore(cr);
      }
    }
  }
}