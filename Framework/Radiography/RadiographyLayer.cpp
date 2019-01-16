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


#include "RadiographyLayer.h"

#include <Core/OrthancException.h>


namespace OrthancStone
{
  static double Square(double x)
  {
    return x * x;
  }


  RadiographyLayer::Geometry::Geometry() :
    hasCrop_(false),
    panX_(0),
    panY_(0),
    angle_(0),
    resizeable_(false),
    pixelSpacingX_(1),
    pixelSpacingY_(1)
  {

  }

  void RadiographyLayer::Geometry::GetCrop(unsigned int &x, unsigned int &y, unsigned int &width, unsigned int &height) const
  {
    if (!hasCrop_)
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);  // you should probably use RadiographyLayer::GetCrop() or at least call HasCrop() before

    x = cropX_;
    y = cropY_;
    width = cropWidth_;
    height = cropHeight_;
  }

  void RadiographyLayer::UpdateTransform()
  {
    transform_ = AffineTransform2D::CreateScaling(geometry_.GetPixelSpacingX(), geometry_.GetPixelSpacingY());

    double centerX, centerY;
    GetCenter(centerX, centerY);

    transform_ = AffineTransform2D::Combine(
          AffineTransform2D::CreateOffset(geometry_.GetPanX() + centerX, geometry_.GetPanY() + centerY),
          AffineTransform2D::CreateRotation(geometry_.GetAngle()),
          AffineTransform2D::CreateOffset(-centerX, -centerY),
          transform_);

    transformInverse_ = AffineTransform2D::Invert(transform_);
  }


  void RadiographyLayer::AddToExtent(Extent2D& extent,
                                     double x,
                                     double y) const
  {
    transform_.Apply(x, y);
    extent.AddPoint(x, y);
  }


  void RadiographyLayer::GetCornerInternal(double& x,
                                           double& y,
                                           Corner corner,
                                           unsigned int cropX,
                                           unsigned int cropY,
                                           unsigned int cropWidth,
                                           unsigned int cropHeight) const
  {
    double dx = static_cast<double>(cropX);
    double dy = static_cast<double>(cropY);
    double dwidth = static_cast<double>(cropWidth);
    double dheight = static_cast<double>(cropHeight);

    switch (corner)
    {
    case Corner_TopLeft:
      x = dx;
      y = dy;
      break;

    case Corner_TopRight:
      x = dx + dwidth;
      y = dy;
      break;

    case Corner_BottomLeft:
      x = dx;
      y = dy + dheight;
      break;

    case Corner_BottomRight:
      x = dx + dwidth;
      y = dy + dheight;
      break;

    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    transform_.Apply(x, y);
  }


  bool RadiographyLayer::Contains(double x,
                                  double y) const
  {
    transformInverse_.Apply(x, y);

    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);

    return (x >= cropX && x <= cropX + cropWidth &&
            y >= cropY && y <= cropY + cropHeight);
  }


  void RadiographyLayer::DrawBorders(CairoContext& context,
                                     double zoom)
  {
    unsigned int cx, cy, width, height;
    GetCrop(cx, cy, width, height);

    double dx = static_cast<double>(cx);
    double dy = static_cast<double>(cy);
    double dwidth = static_cast<double>(width);
    double dheight = static_cast<double>(height);

    cairo_t* cr = context.GetObject();
    cairo_set_line_width(cr, 2.0 / zoom);

    double x, y;
    x = dx;
    y = dy;
    transform_.Apply(x, y);
    cairo_move_to(cr, x, y);

    x = dx + dwidth;
    y = dy;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    x = dx + dwidth;
    y = dy + dheight;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    x = dx;
    y = dy + dheight;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    x = dx;
    y = dy;
    transform_.Apply(x, y);
    cairo_line_to(cr, x, y);

    cairo_stroke(cr);
  }


  RadiographyLayer::RadiographyLayer() :
    index_(0),
    hasSize_(false),
    width_(0),
    height_(0),
    prefferedPhotometricDisplayMode_(PhotometricDisplayMode_Default)
  {
    UpdateTransform();
  }

  void RadiographyLayer::ResetCrop()
  {
    geometry_.ResetCrop();
    UpdateTransform();
  }

  void RadiographyLayer::SetCrop(unsigned int x,
                                 unsigned int y,
                                 unsigned int width,
                                 unsigned int height)
  {
    if (!hasSize_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    if (x + width > width_ ||
        y + height > height_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    geometry_.SetCrop(x, y, width, height);
    UpdateTransform();
  }

  void RadiographyLayer::SetGeometry(const Geometry& geometry)
  {
    geometry_ = geometry;

    if (hasSize_)
    {
      UpdateTransform();
    }
  }


  void RadiographyLayer::GetCrop(unsigned int& x,
                                 unsigned int& y,
                                 unsigned int& width,
                                 unsigned int& height) const
  {
    if (GetGeometry().HasCrop())
    {
      GetGeometry().GetCrop(x, y, width, height);
    }
    else 
    {
      x = 0;
      y = 0;
      width = width_;
      height = height_;
    }
  }

  
  void RadiographyLayer::SetAngle(double angle)
  {
    geometry_.SetAngle(angle);
    UpdateTransform();
  }


  void RadiographyLayer::SetSize(unsigned int width,
                                 unsigned int height)
  {
    if (hasSize_ &&
        (width != width_ ||
         height != height_))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
    }

    hasSize_ = true;
    width_ = width;
    height_ = height;

    UpdateTransform();
  }


  Extent2D RadiographyLayer::GetExtent() const
  {
    Extent2D extent;

    unsigned int x, y, width, height;
    GetCrop(x, y, width, height);

    double dx = static_cast<double>(x);
    double dy = static_cast<double>(y);
    double dwidth = static_cast<double>(width);
    double dheight = static_cast<double>(height);

    AddToExtent(extent, dx, dy);
    AddToExtent(extent, dx + dwidth, dy);
    AddToExtent(extent, dx, dy + dheight);
    AddToExtent(extent, dx + dwidth, dy + dheight);

    return extent;
  }


  bool RadiographyLayer::GetPixel(unsigned int& imageX,
                                  unsigned int& imageY,
                                  double sceneX,
                                  double sceneY) const
  {
    if (width_ == 0 ||
        height_ == 0)
    {
      return false;
    }
    else
    {
      transformInverse_.Apply(sceneX, sceneY);

      int x = static_cast<int>(std::floor(sceneX));
      int y = static_cast<int>(std::floor(sceneY));

      if (x < 0)
      {
        imageX = 0;
      }
      else if (x >= static_cast<int>(width_))
      {
        imageX = width_;
      }
      else
      {
        imageX = static_cast<unsigned int>(x);
      }

      if (y < 0)
      {
        imageY = 0;
      }
      else if (y >= static_cast<int>(height_))
      {
        imageY = height_;
      }
      else
      {
        imageY = static_cast<unsigned int>(y);
      }

      return true;
    }
  }


  void RadiographyLayer::SetPan(double x,
                                double y)
  {
    geometry_.SetPan(x, y);
    UpdateTransform();
  }


  void RadiographyLayer::SetPixelSpacing(double x,
                                         double y)
  {
    geometry_.SetPixelSpacing(x, y);
    UpdateTransform();
  }


  void RadiographyLayer::GetCenter(double& centerX,
                                   double& centerY) const
  {
    centerX = static_cast<double>(width_) / 2.0;
    centerY = static_cast<double>(height_) / 2.0;
    transform_.Apply(centerX, centerY);
  }


  void RadiographyLayer::GetCorner(double& x /* out */,
                                   double& y /* out */,
                                   Corner corner) const
  {
    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);
    GetCornerInternal(x, y, corner, cropX, cropY, cropWidth, cropHeight);
  }


  bool RadiographyLayer::LookupCorner(Corner& corner /* out */,
                                      double x,
                                      double y,
                                      double zoom,
                                      double viewportDistance) const
  {
    static const Corner CORNERS[] = {
      Corner_TopLeft,
      Corner_TopRight,
      Corner_BottomLeft,
      Corner_BottomRight
    };

    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);

    double threshold = Square(viewportDistance / zoom);

    for (size_t i = 0; i < 4; i++)
    {
      double cx, cy;
      GetCornerInternal(cx, cy, CORNERS[i], cropX, cropY, cropWidth, cropHeight);

      double d = Square(cx - x) + Square(cy - y);

      if (d <= threshold)
      {
        corner = CORNERS[i];
        return true;
      }
    }

    return false;
  }
}
