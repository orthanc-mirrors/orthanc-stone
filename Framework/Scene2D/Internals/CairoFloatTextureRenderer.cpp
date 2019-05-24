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


#include "CairoFloatTextureRenderer.h"

#include "../FloatTextureSceneLayer.h"

namespace OrthancStone
{
  namespace Internals
  {
    void CairoFloatTextureRenderer::Update(const ISceneLayer& layer)
    {
      const FloatTextureSceneLayer& l = dynamic_cast<const FloatTextureSceneLayer&>(layer);

      textureTransform_ = l.GetTransform();
      isLinearInterpolation_ = l.IsLinearInterpolation();

      float windowCenter, windowWidth;
      l.GetWindowing(windowCenter, windowWidth);

      const float a = windowCenter - windowWidth / 2.0f;
      const float slope = 256.0f / windowWidth;

      const Orthanc::ImageAccessor& source = l.GetTexture();
      const unsigned int width = source.GetWidth();
      const unsigned int height = source.GetHeight();
      texture_.SetSize(width, height, false);

      Orthanc::ImageAccessor target;
      texture_.GetWriteableAccessor(target);

      assert(source.GetFormat() == Orthanc::PixelFormat_Float32 &&
             target.GetFormat() == Orthanc::PixelFormat_BGRA32 &&
             sizeof(float) == 4);

      for (unsigned int y = 0; y < height; y++)
      {
        const float* p = reinterpret_cast<const float*>(source.GetConstRow(y));
        uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));

        for (unsigned int x = 0; x < width; x++)
        {
          float v = (*p - a) * slope;
          if (v <= 0)
          {
            v = 0;
          }
          else if (v >= 255)
          {
            v = 255;
          }

          uint8_t vv = static_cast<uint8_t>(v);

          q[0] = vv;
          q[1] = vv;
          q[2] = vv;

          p++;
          q += 4;
        }
      }
    }

      
    void CairoFloatTextureRenderer::Render(const AffineTransform2D& transform)
    {
      cairo_t* cr = target_.GetCairoContext();

      AffineTransform2D t =
        AffineTransform2D::Combine(transform, textureTransform_);
      Matrix h = t.GetHomogeneousMatrix();
      
      cairo_save(cr);

      cairo_matrix_t m;
      cairo_matrix_init(&m, h(0, 0), h(1, 0), h(0, 1), h(1, 1), h(0, 2), h(1, 2));
      cairo_transform(cr, &m);

      cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
      cairo_set_source_surface(cr, texture_.GetObject(), 0, 0);

      if (isLinearInterpolation_)
      {
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
      }
      else
      {
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
      }

      cairo_paint(cr);

      cairo_restore(cr);
    }
  }
}
