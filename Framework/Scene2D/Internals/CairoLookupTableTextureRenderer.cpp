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


#include "CairoLookupTableTextureRenderer.h"

#include "CairoColorTextureRenderer.h"
#include "../LookupTableTextureSceneLayer.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  namespace Internals
  {
    void CairoLookupTableTextureRenderer::Update(const ISceneLayer& layer)
    {
      const LookupTableTextureSceneLayer& l = dynamic_cast<const LookupTableTextureSceneLayer&>(layer);

      textureTransform_ = l.GetTransform();
      isLinearInterpolation_ = l.IsLinearInterpolation();

      const float a = l.GetMinValue();
      float slope;

      if (l.GetMinValue() >= l.GetMaxValue())
      {
        slope = 0;
      }
      else
      {
        slope = 256.0f / (l.GetMaxValue() - l.GetMinValue());
      }

      const Orthanc::ImageAccessor& source = l.GetTexture();
      const unsigned int width = source.GetWidth();
      const unsigned int height = source.GetHeight();
      texture_.SetSize(width, height, true /* alpha channel is enabled */);

      Orthanc::ImageAccessor target;
      texture_.GetWriteableAccessor(target);

      const std::vector<uint8_t>& lut = l.GetLookupTable();
      if (lut.size() != 4 * 256)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

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

          if (1) //l.IsApplyLog())
          {
            // https://theailearner.com/2019/01/01/log-transformation/
            v = 255.0f / log(1.0f + 255.0f * 1.5f) * log(1.0f + static_cast<float>(v));
            if (v <= 0)
            {
              v = 0;
            }
            else if (v >= 255)
            {
              v = 255;
            }
          }

          uint8_t vv = static_cast<uint8_t>(v);

          q[0] = lut[4 * vv + 2];  // B
          q[1] = lut[4 * vv + 1];  // G
          q[2] = lut[4 * vv + 0];  // R
          q[3] = lut[4 * vv + 3];  // A

          p++;
          q += 4;
        }
      }

      cairo_surface_mark_dirty(texture_.GetObject());
    }

    void CairoLookupTableTextureRenderer::Render(const AffineTransform2D& transform,
                                                 unsigned int canvasWidth,
                                                 unsigned int canvasHeight)
    {
      CairoColorTextureRenderer::RenderColorTexture(target_, transform, texture_,
                                                    textureTransform_, isLinearInterpolation_);
    }
  }
}
