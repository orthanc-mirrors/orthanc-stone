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


#include "RadiographyMaskLayer.h"
#include "RadiographyDicomLayer.h"

#include "RadiographyScene.h"
#include "Core/Images/Image.h"
#include "Core/Images/ImageProcessing.h"
#include <Core/OrthancException.h>
#include "../Toolbox/ImageGeometry.h"

namespace OrthancStone
{
  const unsigned char IN_MASK_VALUE = 0x77;
  const unsigned char OUT_MASK_VALUE = 0xFF;

  const AffineTransform2D& RadiographyMaskLayer::GetTransform() const
  {
    return dicomLayer_.GetTransform();
  }

  const AffineTransform2D& RadiographyMaskLayer::GetTransformInverse() const
  {
    return dicomLayer_.GetTransformInverse();
  }

  bool RadiographyMaskLayer::GetPixel(unsigned int& imageX,
                        unsigned int& imageY,
                        double sceneX,
                        double sceneY) const
  {
    return dicomLayer_.GetPixel(imageX, imageY, sceneX, sceneY);
  }

  std::string RadiographyMaskLayer::GetInstanceId() const
  {
    return dicomLayer_.GetInstanceId();
  }

  void RadiographyMaskLayer::SetCorner(const Orthanc::ImageProcessing::ImagePoint& corner, size_t index)
  {
    if (index < corners_.size())
      corners_[index] = corner;
    else
      corners_.push_back(corner);
    invalidated_ = true;

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyMaskLayer::SetCorners(const std::vector<Orthanc::ImageProcessing::ImagePoint>& corners)
  {
    corners_ = corners;
    invalidated_ = true;

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  Extent2D RadiographyMaskLayer::GetSceneExtent(bool minimal) const
  {
    if (!minimal)
    {
      return RadiographyLayer::GetSceneExtent(minimal);
    }
    else
    { // get the extent of the in-mask area
      Extent2D sceneExtent;

      for (std::vector<Orthanc::ImageProcessing::ImagePoint>::const_iterator& corner = corners_.begin(); corner != corners_.end(); corner++)
      {
        double x = static_cast<double>(corner->GetX());
        double y = static_cast<double>(corner->GetY());

        dicomLayer_.GetTransform().Apply(x, y);
        sceneExtent.AddPoint(x, y);
      }
      return sceneExtent;
    }
  }



  void RadiographyMaskLayer::Render(Orthanc::ImageAccessor& buffer,
                                    const AffineTransform2D& viewTransform,
                                    ImageInterpolation interpolation,
                                    float windowCenter,
                                    float windowWidth,
                                    bool applyWindowing) const
  {
    if (dicomLayer_.GetWidth() == 0 || dicomLayer_.GetSourceImage() == NULL) // nothing to do if the DICOM layer is not displayed (or not loaded)
      return;

    if (invalidated_)
    {
      mask_.reset(new Orthanc::Image(Orthanc::PixelFormat_Grayscale8, dicomLayer_.GetWidth(), dicomLayer_.GetHeight(), false));

      DrawMask();

      invalidated_ = false;
    }

    {// rendering
      if (buffer.GetFormat() != Orthanc::PixelFormat_Float32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }

      unsigned int cropX, cropY, cropWidth, cropHeight;
      dicomLayer_.GetCrop(cropX, cropY, cropWidth, cropHeight);

      const AffineTransform2D t = AffineTransform2D::Combine(
            viewTransform, dicomLayer_.GetTransform(),
            AffineTransform2D::CreateOffset(cropX, cropY));

      Orthanc::ImageAccessor cropped;
      mask_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);

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

      t.Apply(tmp, cropped, ImageInterpolation_Nearest, true /* clear */);

      // we have observed vertical lines at the image border (probably due to bilinear filtering of the DICOM image when it is not aligned with the buffer pixels)
      // -> draw the mask one line further on each side
      if (x1 >= 1)
      {
        x1 = x1 - 1;
      }
      if (x2 < buffer.GetWidth() - 2)
      {
        x2 = x2 + 1;
      }

      // Blit
      for (unsigned int y = y1; y <= y2; y++)
      {
        float *q = reinterpret_cast<float*>(buffer.GetRow(y)) + x1;
        const uint8_t *p = reinterpret_cast<uint8_t*>(tmp.GetRow(y)) + x1;

        for (unsigned int x = x1; x <= x2; x++, p++, q++)
        {
          if (*p != IN_MASK_VALUE)
            *q = foreground_;
          // else keep the underlying pixel value
        }
      }

    }
  }

  void RadiographyMaskLayer::DrawMask() const
  {
    // first fill the complete image
    Orthanc::ImageProcessing::Set(*mask_, OUT_MASK_VALUE);

    // clip corners
    std::vector<Orthanc::ImageProcessing::ImagePoint> clippedCorners;
    for (size_t i = 0; i < corners_.size(); i++)
    {
      clippedCorners.push_back(corners_[i]);
      clippedCorners[i].ClipTo(0, mask_->GetWidth() - 1, 0, mask_->GetHeight() - 1);
    }

    // fill mask
    Orthanc::ImageProcessing::FillPolygon(*mask_, clippedCorners, IN_MASK_VALUE);

  }

}
