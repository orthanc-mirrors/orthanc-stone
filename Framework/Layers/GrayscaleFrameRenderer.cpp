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


#include "GrayscaleFrameRenderer.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  CairoSurface* GrayscaleFrameRenderer::GenerateDisplay(const RenderStyle& style)
  {
    std::auto_ptr<CairoSurface> result;

    float windowCenter, windowWidth;
    style.ComputeWindowing(windowCenter, windowWidth,
                           defaultWindowCenter_, defaultWindowWidth_);

    float x0 = windowCenter - windowWidth / 2.0f;
    float x1 = windowCenter + windowWidth / 2.0f;

    //LOG(INFO) << "Window: " << x0 << " => " << x1;

    result.reset(new CairoSurface(frame_->GetWidth(), frame_->GetHeight()));

    const uint8_t* lut = NULL;
    if (style.applyLut_)
    {
      if (Orthanc::EmbeddedResources::GetFileResourceSize(style.lut_) != 3 * 256)
      {
        // Invalid colormap
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      lut = reinterpret_cast<const uint8_t*>(Orthanc::EmbeddedResources::GetFileResourceBuffer(style.lut_));
    }
      
    Orthanc::ImageAccessor target = result->GetAccessor();
    for (unsigned int y = 0; y < target.GetHeight(); y++)
    {
      const float* p = reinterpret_cast<const float*>(frame_->GetConstRow(y));
      uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));

      for (unsigned int x = 0; x < target.GetWidth(); x++, p++, q += 4)
      {
        uint8_t v = 0;
        if (windowWidth >= 0.001f)  // Avoid division by zero
        {
          if (*p >= x1)
          {
            v = 255;
          }
          else if (*p <= x0)
          {
            v = 0;
          }
          else
          {
            // https://en.wikipedia.org/wiki/Linear_interpolation
            v = static_cast<uint8_t>(255.0f * (*p - x0) / (x1 - x0));
          }

          if (style.reverse_)
          {
            v = 255 - v;
          }
        }

        if (style.applyLut_)
        {
          assert(lut != NULL);
          q[3] = 255;
          q[2] = lut[3 * v];
          q[1] = lut[3 * v + 1];
          q[0] = lut[3 * v + 2];
        }
        else
        {
          q[3] = 255;
          q[2] = v;
          q[1] = v;
          q[0] = v;
        }
      }
    }

    return result.release();
  }


  GrayscaleFrameRenderer::GrayscaleFrameRenderer(Orthanc::ImageAccessor* frame,
                                                 const DicomFrameConverter& converter,
                                                 const SliceGeometry& frameSlice,
                                                 double pixelSpacingX,
                                                 double pixelSpacingY,
                                                 bool isFullQuality) :
    FrameRenderer(frameSlice, pixelSpacingX, pixelSpacingY, isFullQuality),
    frame_(frame),
    defaultWindowCenter_(converter.GetDefaultWindowCenter()),
    defaultWindowWidth_(converter.GetDefaultWindowWidth())
  {
    if (frame == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    converter.ConvertFrame(frame_);
    assert(frame_.get() != NULL);

    if (frame_->GetFormat() != Orthanc::PixelFormat_Float32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }
  }
}
