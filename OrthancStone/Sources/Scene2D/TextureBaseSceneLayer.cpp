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


#include "TextureBaseSceneLayer.h"

#include <OrthancException.h>

namespace OrthancStone
{
  void TextureBaseSceneLayer::SetTexture(Orthanc::ImageAccessor* texture)
  {
    if (texture == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      texture_.reset(texture);
      IncrementRevision();
    }
  }


  void TextureBaseSceneLayer::CopyParameters(const TextureBaseSceneLayer& other)
  {
    originX_ = other.originX_;
    originY_ = other.originY_;
    pixelSpacingX_ = other.pixelSpacingX_;
    pixelSpacingY_ = other.pixelSpacingY_;
    angle_ = other.angle_;
    isLinearInterpolation_ = other.isLinearInterpolation_;
    flipX_ = other.flipX_;
    flipY_ = other.flipY_;
  }


  TextureBaseSceneLayer::TextureBaseSceneLayer() :
    originX_(0),
    originY_(0),
    pixelSpacingX_(1),
    pixelSpacingY_(1),
    angle_(0),
    isLinearInterpolation_(false),
    flipX_(false),
    flipY_(false),
    revision_(0)
  {
    if (pixelSpacingX_ <= 0 ||
        pixelSpacingY_ <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  
  void TextureBaseSceneLayer::SetOrigin(double x,
                                        double y)
  {
    originX_ = x;
    originY_ = y;
    IncrementRevision();
  }


  void TextureBaseSceneLayer::SetPixelSpacing(double sx,
                                              double sy)
  {
    if (sx <= 0 ||
        sy <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      pixelSpacingX_ = sx;
      pixelSpacingY_ = sy;
      IncrementRevision();
    }
  }

  
  void TextureBaseSceneLayer::SetAngle(double angle)
  {
    angle_ = angle;
    IncrementRevision();
  }

  
  void TextureBaseSceneLayer::SetLinearInterpolation(bool isLinearInterpolation)
  {
    isLinearInterpolation_ = isLinearInterpolation;
    IncrementRevision();
  }
    

  void TextureBaseSceneLayer::SetFlipX(bool flip)
  {
    flipX_ = flip;
    IncrementRevision();
  }
  
    
  void TextureBaseSceneLayer::SetFlipY(bool flip)
  {
    flipY_ = flip;
    IncrementRevision();
  }
    

  const Orthanc::ImageAccessor& TextureBaseSceneLayer::GetTexture() const
  {
    if (!HasTexture())
    {
      LOG(ERROR) << "TextureBaseSceneLayer::GetTexture(): (!HasTexture())";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return *texture_;
    }
  }

  
  AffineTransform2D TextureBaseSceneLayer::GetTransform() const
  {
    unsigned int width = 0;
    unsigned int height = 0;

    if (texture_.get() != NULL)
    {
      width = texture_->GetWidth();
      height = texture_->GetHeight();
    }
    
    return AffineTransform2D::Combine(
      AffineTransform2D::CreateOffset(originX_, originY_),
      AffineTransform2D::CreateRotation(angle_),
      AffineTransform2D::CreateScaling(pixelSpacingX_, pixelSpacingY_),
      AffineTransform2D::CreateOffset(-0.5, -0.5),
      AffineTransform2D::CreateFlip(flipX_, flipY_, width, height));
  }

  
  bool TextureBaseSceneLayer::GetBoundingBox(Extent2D& target) const
  {
    if (texture_.get() == NULL)
    {
      return false;
    }
    else
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
}
