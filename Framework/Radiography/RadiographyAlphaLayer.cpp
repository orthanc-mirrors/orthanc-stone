/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include <Core/Images/Image.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{

  void RadiographyAlphaLayer::SetAlpha(Orthanc::ImageAccessor* image)
  {
    std::auto_ptr<Orthanc::ImageAccessor> raii(image);

    if (image == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (image->GetFormat() != Orthanc::PixelFormat_Grayscale8)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    SetSize(image->GetWidth(), image->GetHeight());
    alpha_ = raii;

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyAlphaLayer::Render(Orthanc::ImageAccessor& buffer,
                                     const AffineTransform2D& viewTransform,
                                     ImageInterpolation interpolation) const
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

    t.Apply(tmp, cropped, interpolation, true /* clear */);

    // Blit
    const unsigned int width = buffer.GetWidth();
    const unsigned int height = buffer.GetHeight();

    float value = foreground_;

    if (useWindowing_)
    {
      float center, width;
      if (GetScene().GetWindowing(center, width))
      {
        value = center + width / 2.0f;  // set it to the maximum pixel value of the image
      }
    }

    for (unsigned int y = 0; y < height; y++)
    {
      float *q = reinterpret_cast<float*>(buffer.GetRow(y));
      const uint8_t *p = reinterpret_cast<uint8_t*>(tmp.GetRow(y));

      for (unsigned int x = 0; x < width; x++, p++, q++)
      {
        float a = static_cast<float>(*p) / 255.0f;

        *q = (a * value + (1.0f - a) * (*q));
      }
    }
  }

  bool RadiographyAlphaLayer::GetRange(float& minValue,
                                       float& maxValue) const
  {
    if (useWindowing_)
    {
      return false;
    }
    else
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
}
