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


#include "OpenGLLookupTableTextureRenderer.h"

namespace OrthancStone
{
  namespace Internals
  {
    void OpenGLLookupTableTextureRenderer::LoadTexture(
      const LookupTableTextureSceneLayer& layer)
    {
      const Orthanc::ImageAccessor& source = layer.GetTexture();
      const unsigned int width = source.GetWidth();
      const unsigned int height = source.GetHeight();

      if ((texture_.get() == NULL) || 
          (texture_->GetWidth() != width) || 
          (texture_->GetHeight() != height))
      {

        texture_.reset(new Orthanc::Image(
          Orthanc::PixelFormat_RGBA32,
          width,
          height,
          false));
      }

      {

        const float a = layer.GetMinValue();
        float slope = 0;

        if (layer.GetMinValue() >= layer.GetMaxValue())
        {
          slope = 0;
        }
        else
        {
          slope = 256.0f / (layer.GetMaxValue() - layer.GetMinValue());
        }

        Orthanc::ImageAccessor target;
        texture_->GetWriteableAccessor(target);

        const std::vector<uint8_t>& lut = layer.GetLookupTable();
        if (lut.size() != 4 * 256)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        assert(source.GetFormat() == Orthanc::PixelFormat_Float32 &&
          target.GetFormat() == Orthanc::PixelFormat_RGBA32 &&
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

            q[0] = lut[4 * vv + 0];  // R
            q[1] = lut[4 * vv + 1];  // G
            q[2] = lut[4 * vv + 2];  // B
            q[3] = lut[4 * vv + 3];  // A

            p++;
            q += 4;
          }
        }
      }

      context_.MakeCurrent();
      glTexture_.reset(new OpenGL::OpenGLTexture);
      glTexture_->Load(*texture_, layer.IsLinearInterpolation());
      layerTransform_ = layer.GetTransform();
    }

    
    OpenGLLookupTableTextureRenderer::OpenGLLookupTableTextureRenderer(
      OpenGL::IOpenGLContext&                 context,
      OpenGLColorTextureProgram&              program,
      const LookupTableTextureSceneLayer&     layer)
      : context_(context)
      , program_(program)
    {
      LoadTexture(layer);
    }

    
    void OpenGLLookupTableTextureRenderer::Render(const AffineTransform2D& transform)
    {
      if (glTexture_.get() != NULL)
      {
        program_.Apply(
          *glTexture_, 
          AffineTransform2D::Combine(transform, layerTransform_), 
          true);
      }
    }

    
    void OpenGLLookupTableTextureRenderer::Update(const ISceneLayer& layer)
    {
      // Should never happen (no revisions in color textures)
      LoadTexture(dynamic_cast<const LookupTableTextureSceneLayer&>(layer));
    }
  }
}
