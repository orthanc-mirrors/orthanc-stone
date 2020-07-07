/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "RadiographyAlphaLayer.h"

#include "RadiographyScene.h"
#include "../Toolbox/ImageGeometry.h"

#include <Compatibility.h>
#include <Images/Image.h>
#include <OrthancException.h>


namespace OrthancStone
{

  void RadiographyAlphaLayer::SetAlpha(Orthanc::ImageAccessor* image)
  {
    std::unique_ptr<Orthanc::ImageAccessor> raii(image);

    if (image == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (image->GetFormat() != Orthanc::PixelFormat_Grayscale8)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    SetSize(image->GetWidth(), image->GetHeight());

#if __cplusplus < 201103L
    alpha_.reset(raii.release());
#else
    alpha_ = std::move(raii);
#endif

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyAlphaLayer::Render(Orthanc::ImageAccessor& buffer,
                                     const AffineTransform2D& viewTransform,
                                     ImageInterpolation interpolation,
                                     float windowCenter,
                                     float windowWidth,
                                     bool applyWindowing) const
  {
    if (alpha_.get() == NULL)
    {
      return;
    }

    if (buffer.GetFormat() != Orthanc::PixelFormat_Float32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);

    const AffineTransform2D t = AffineTransform2D::Combine(
          viewTransform, GetTransform(),
          AffineTransform2D::CreateOffset(cropX, cropY));

    Orthanc::ImageAccessor cropped;
    alpha_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);

    Orthanc::Image tmp(Orthanc::PixelFormat_Grayscale8, buffer.GetWidth(), buffer.GetHeight(), false);

    unsigned int x1, y1, x2, y2;

    if (!OrthancStone::GetProjectiveTransformExtent(x1, y1, x2, y2,
                                                    t.GetHomogeneousMatrix(),
                                                    cropped.GetWidth(),
                                                    cropped.GetHeight(),
                                                    buffer.GetWidth(),
                                                    buffer.GetHeight()))
    {
      return;  // layer is outside the buffer
    }

    t.Apply(tmp, cropped, interpolation, true /* clear */);

    float value = foreground_;

    if (!applyWindowing) // if applying the windowing, it means we are ie rendering the image for a realtime visualization -> the foreground_ value is the value we want to see on the screen -> don't change it
    {
      // if not applying the windowing, it means ie that we are saving a dicom image to file and the windowing will be applied by a viewer later on -> we want the "foreground" value to be correct once the windowing will be applied
      value = windowCenter - windowWidth/2 + (foreground_ / 65535.0f) * windowWidth;

      if (value < 0.0f)
      {
        value = 0.0f;
      }
      if (value > 65535.0f)
      {
        value = 65535.0f;
      }
    }

    for (unsigned int y = y1; y <= y2; y++)
    {
      float *q = reinterpret_cast<float*>(buffer.GetRow(y)) + x1;
      const uint8_t *p = reinterpret_cast<uint8_t*>(tmp.GetRow(y)) + x1;

      for (unsigned int x = x1; x <= x2; x++, p++, q++)
      {
        float a = static_cast<float>(*p) / 255.0f;

        *q = (a * value + (1.0f - a) * (*q));
      }
    }
  }

  bool RadiographyAlphaLayer::GetRange(float& minValue,
                                       float& maxValue) const
  {
    minValue = 0;
    maxValue = 0;

    if (foreground_ < 0)
    {
      minValue = foreground_;
    }

    if (foreground_ > 0)
    {
      maxValue = foreground_;
    }

    return true;
  }
}
