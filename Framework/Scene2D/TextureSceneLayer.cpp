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


#include "TextureSceneLayer.h"

#include <Core/Images/Image.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  TextureSceneLayer::TextureSceneLayer(const Orthanc::ImageAccessor& texture,
                                       double originX,  // Center of the top-left pixel
                                       double originY,
                                       double pixelSpacingX,
                                       double pixelSpacingY,
                                       double angle,
                                       bool isLinearInterpolation) :
    texture_(Orthanc::Image::Clone(texture)),
    originX_(originX),
    originY_(originY),
    pixelSpacingX_(pixelSpacingX),
    pixelSpacingY_(pixelSpacingY),
    angle_(angle),
    isLinearInterpolation_(isLinearInterpolation)
  {
    if (texture_->GetFormat() != Orthanc::PixelFormat_Grayscale8 &&
        texture_->GetFormat() != Orthanc::PixelFormat_RGBA32 &&
        texture_->GetFormat() != Orthanc::PixelFormat_RGB24)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    if (pixelSpacingX_ <= 0 ||
        pixelSpacingY_ <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  ISceneLayer* TextureSceneLayer::Clone() const
  {
    return new TextureSceneLayer(*texture_, originX_, originY_, 
                                 pixelSpacingX_, pixelSpacingY_, angle_, 
                                 isLinearInterpolation_);
  }


  AffineTransform2D TextureSceneLayer::GetTransform() const
  {
    return AffineTransform2D::Combine(
      AffineTransform2D::CreateOffset(originX_, originY_),
      AffineTransform2D::CreateRotation(angle_),
      AffineTransform2D::CreateScaling(pixelSpacingX_, pixelSpacingY_),
      AffineTransform2D::CreateOffset(-0.5, -0.5));
  }


  bool TextureSceneLayer::GetBoundingBox(Extent2D& target) const
  {
    const AffineTransform2D t = GetTransform();

    target.Reset();
    
    double x, y;

    x = 0;
    y = 0;
    t.Apply(x, y);
    target.AddPoint(x, y);

    x = static_cast<double>(texture_->GetWidth());
    y = 0;
    t.Apply(x, y);
    target.AddPoint(x, y);

    x = 0;
    y = static_cast<double>(texture_->GetHeight());
    t.Apply(x, y);
    target.AddPoint(x, y);

    x = static_cast<double>(texture_->GetWidth());
    y = static_cast<double>(texture_->GetHeight());
    t.Apply(x, y);
    target.AddPoint(x, y);    

    return true;
  }
}
