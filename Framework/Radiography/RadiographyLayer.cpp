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
    flipVertical_(false),
    flipHorizontal_(false),
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
    // important to update transform_ before getting the center to use the right scaling !!!
    transform_ = AffineTransform2D::CreateScaling(geometry_.GetScalingX(), geometry_.GetScalingY());

    double centerX, centerY;
    GetCenter(centerX, centerY);

    transform_ = AffineTransform2D::Combine(
          AffineTransform2D::CreateOffset(geometry_.GetPanX(), geometry_.GetPanY()),
          AffineTransform2D::CreateRotation(geometry_.GetAngle(), centerX, centerY),
          transform_);

    transformInverse_ = AffineTransform2D::Invert(transform_);
  }


  void RadiographyLayer::AddToExtent(Extent2D& extent,
                                     double x,
                                     double y) const
  {
    GetTransform().Apply(x, y);
    extent.AddPoint(x, y);
  }

  bool RadiographyLayer::Contains(double x,
                                  double y) const
  {
    GetTransformInverse().Apply(x, y);

    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);

    return (x >= cropX && x <= cropX + cropWidth &&
            y >= cropY && y <= cropY + cropHeight);
  }


  void RadiographyLayer::DrawBorders(CairoContext& context,
                                     double zoom)
  {
    if (GetControlPointCount() < 3 )
      return;

    cairo_t* cr = context.GetObject();
    cairo_set_line_width(cr, 2.0 / zoom);

    ControlPoint cp;
    GetControlPoint(cp, 0);
    cairo_move_to(cr, cp.x, cp.y);

    for (size_t i = 0; i < GetControlPointCount(); i++)
    {
      GetControlPoint(cp, i);
      cairo_line_to(cr, cp.x, cp.y);
    }

    cairo_close_path(cr);
    cairo_stroke(cr);
  }


  RadiographyLayer::RadiographyLayer(MessageBroker& broker, const RadiographyScene& scene) :
    IObservable(broker),
    index_(0),
    hasSize_(false),
    width_(0),
    height_(0),
    prefferedPhotometricDisplayMode_(RadiographyPhotometricDisplayMode_Default),
    scene_(scene)
  {
    UpdateTransform();
  }

  void RadiographyLayer::ResetCrop()
  {
    geometry_.ResetCrop();
    UpdateTransform();
  }

  void RadiographyLayer::SetPreferredPhotomotricDisplayMode(RadiographyPhotometricDisplayMode  prefferedPhotometricDisplayMode)
  {
    prefferedPhotometricDisplayMode_ = prefferedPhotometricDisplayMode;

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
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

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyLayer::SetGeometry(const Geometry& geometry)
  {
    geometry_ = geometry;

    if (hasSize_)
    {
      UpdateTransform();
    }

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
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

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyLayer::SetFlipVertical(bool flip)
  {
    geometry_.SetFlipVertical(flip);
    UpdateTransform();

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyLayer::SetFlipHorizontal(bool flip)
  {
    geometry_.SetFlipHorizontal(flip);
    UpdateTransform();

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyLayer::SetSize(unsigned int width,
                                 unsigned int height,
                                 bool emitLayerEditedEvent)
  {
    hasSize_ = true;
    width_ = width;
    height_ = height;

    UpdateTransform();

    if (emitLayerEditedEvent)
    {
      BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
    }
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
      GetTransformInverse().Apply(sceneX, sceneY);

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
    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }


  void RadiographyLayer::SetPixelSpacing(double x,
                                         double y,
                                         bool emitLayerEditedEvent)
  {
    geometry_.SetPixelSpacing(x, y);
    UpdateTransform();
    if (emitLayerEditedEvent)
    {
      BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
    }
  }


  void RadiographyLayer::GetCenter(double& centerX,
                                   double& centerY) const
  {
    centerX = static_cast<double>(width_) / 2.0;
    centerY = static_cast<double>(height_) / 2.0;
    GetTransform().Apply(centerX, centerY);
  }



  size_t RadiographyLayer::GetControlPointCount() const {return 4;}

  void RadiographyLayer::GetControlPoint(ControlPoint& cpScene /* out in scene coordinates */,
                                         size_t index) const
  {
    unsigned int cropX, cropY, cropWidth, cropHeight;
    GetCrop(cropX, cropY, cropWidth, cropHeight);

    ControlPoint cp;
    switch (index)
    {
      case RadiographyControlPointType_TopLeftCorner:
        cp = ControlPoint(cropX, cropY, RadiographyControlPointType_TopLeftCorner);
        break;

      case RadiographyControlPointType_TopRightCorner:
        cp = ControlPoint(cropX + cropWidth, cropY, RadiographyControlPointType_TopRightCorner);
        break;

      case RadiographyControlPointType_BottomLeftCorner:
        cp = ControlPoint(cropX, cropY + cropHeight, RadiographyControlPointType_BottomLeftCorner);
        break;

      case RadiographyControlPointType_BottomRightCorner:
        cp = ControlPoint(cropX + cropWidth, cropY + cropHeight, RadiographyControlPointType_BottomRightCorner);
        break;

    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    // transforms image coordinates into scene coordinates
    GetTransform().Apply(cp.x, cp.y);
    cpScene = cp;
  }

  bool RadiographyLayer::LookupControlPoint(ControlPoint& cpScene /* out */,
                                            double x,
                                            double y,
                                            double zoom,
                                            double viewportDistance) const
  {
    double threshold = Square(viewportDistance / zoom);

    for (size_t i = 0; i < GetControlPointCount(); i++)
    {
      ControlPoint cp;
      GetControlPoint(cp, i);

      double d = Square(cp.x - x) + Square(cp.y - y);

      if (d <= threshold)
      {
        cpScene = cp;
        return true;
      }
    }

    return false;
  }
}
