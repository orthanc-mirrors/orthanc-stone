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


#include "GrayscaleFrameRenderer.h"

#include <Core/Images/Image.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  CairoSurface* GrayscaleFrameRenderer::GenerateDisplay(const RenderStyle& style)
  {
    assert(frame_->GetFormat() == Orthanc::PixelFormat_Float32);

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

    Orthanc::ImageAccessor target;
    result->GetWriteableAccessor(target);
    
    const unsigned int width = target.GetWidth();
    const unsigned int height = target.GetHeight();
    
    for (unsigned int y = 0; y < height; y++)
    {
      const float* p = reinterpret_cast<const float*>(frame_->GetConstRow(y));
      uint8_t* q = reinterpret_cast<uint8_t*>(target.GetRow(y));

      for (unsigned int x = 0; x < width; x++, p++, q += 4)
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

          if (style.reverse_ ^ (photometric_ == Orthanc::PhotometricInterpretation_Monochrome1))
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


  GrayscaleFrameRenderer::GrayscaleFrameRenderer(const Orthanc::ImageAccessor& frame,
                                                 const DicomFrameConverter& converter,
                                                 const CoordinateSystem3D& framePlane,
                                                 double pixelSpacingX,
                                                 double pixelSpacingY,
                                                 bool isFullQuality) :
    FrameRenderer(framePlane, pixelSpacingX, pixelSpacingY, isFullQuality),
    frame_(Orthanc::Image::Clone(frame)),
    defaultWindowCenter_(static_cast<float>(converter.GetDefaultWindowCenter())),
    defaultWindowWidth_(static_cast<float>(converter.GetDefaultWindowWidth())),
    photometric_(converter.GetPhotometricInterpretation())
  {
    if (frame_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    converter.ConvertFrameInplace(frame_);
    assert(frame_.get() != NULL);

    if (frame_->GetFormat() != Orthanc::PixelFormat_Float32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }
  }
}
