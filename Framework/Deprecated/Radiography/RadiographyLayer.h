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


#pragma once

#include <algorithm>

#include "../Toolbox/AffineTransform2D.h"
#include "../Toolbox/Extent2D.h"
#include "../Wrappers/CairoContext.h"
#include "../Messages/IMessage.h"
#include "../Messages/IObservable.h"

namespace OrthancStone
{
  class RadiographyScene;

  enum RadiographyControlPointType
  {
    RadiographyControlPointType_TopLeftCorner = 0,
    RadiographyControlPointType_TopRightCorner = 1,
    RadiographyControlPointType_BottomRightCorner = 2,
    RadiographyControlPointType_BottomLeftCorner = 3
  };

  enum RadiographyPhotometricDisplayMode
  {
    RadiographyPhotometricDisplayMode_Default,

    RadiographyPhotometricDisplayMode_Monochrome1,
    RadiographyPhotometricDisplayMode_Monochrome2
  };

  
  struct ControlPoint
  {
    double x;
    double y;
    size_t index;

    ControlPoint(double x, double y, size_t index)
      : x(x),
        y(y),
        index(index)
    {}

    ControlPoint()
      : x(0),
        y(0),
        index(std::numeric_limits<size_t>::max())
    {}
  };

  class RadiographyLayer : public IObservable
  {
    friend class RadiographyScene;

  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, LayerEditedMessage, RadiographyLayer);

    class Geometry
    {
      bool               hasCrop_;
      unsigned int       cropX_;
      unsigned int       cropY_;
      unsigned int       cropWidth_;
      unsigned int       cropHeight_;
      bool               flipVertical_;
      bool               flipHorizontal_;
      double             panX_;
      double             panY_;
      double             angle_;
      bool               resizeable_;
      double             pixelSpacingX_;
      double             pixelSpacingY_;

    public:
      Geometry();

      void ResetCrop()
      {
        hasCrop_ = false;
      }

      void SetCrop(unsigned int x,
                   unsigned int y,
                   unsigned int width,
                   unsigned int height)
      {
        hasCrop_ = true;
        cropX_ = x;
        cropY_ = y;
        cropWidth_ = width;
        cropHeight_ = height;
      }

      bool HasCrop() const
      {
        return hasCrop_;
      }

      void GetCrop(unsigned int& x,
                   unsigned int& y,
                   unsigned int& width,
                   unsigned int& height) const;

      void SetAngle(double angle)
      {
        angle_ = angle;
      }

      double GetAngle() const
      {
        return angle_;
      }

      void SetPan(double x,
                  double y)
      {
        panX_ = x;
        panY_ = y;
      }

      double GetPanX() const
      {
        return panX_;
      }

      double GetPanY() const
      {
        return panY_;
      }

      bool IsResizeable() const
      {
        return resizeable_;
      }

      void SetResizeable(bool resizeable)
      {
        resizeable_ = resizeable;
      }

      void SetPixelSpacing(double x,
                           double y)
      {
        pixelSpacingX_ = x;
        pixelSpacingY_ = y;
      }

      double GetPixelSpacingX() const
      {
        return pixelSpacingX_;
      }

      double GetPixelSpacingY() const
      {
        return pixelSpacingY_;
      }

      void SetFlipVertical(bool flip) //  mirrors image around an horizontal axis (note: flip is applied before the rotation !)
      {
        flipVertical_ = flip;
      }

      void SetFlipHorizontal(bool flip) //  mirrors image around a vertical axis (note: flip is applied before the rotation !)
      {
        flipHorizontal_ = flip;
      }

      bool GetFlipVertical() const
      {
        return flipVertical_;
      }

      bool GetFlipHorizontal() const
      {
        return flipHorizontal_;
      }

      double GetScalingX() const
      {
        return (flipHorizontal_ ? - pixelSpacingX_: pixelSpacingX_);
      }

      double GetScalingY() const
      {
        return (flipVertical_ ? - pixelSpacingY_: pixelSpacingY_);
      }
    };

  private:
    size_t             index_;
    bool               hasSize_;
    unsigned int       width_;
    unsigned int       height_;
    AffineTransform2D  transform_;
    AffineTransform2D  transformInverse_;
    Geometry           geometry_;
    RadiographyPhotometricDisplayMode  prefferedPhotometricDisplayMode_;
    const RadiographyScene&   scene_;

  protected:
    void SetPreferredPhotomotricDisplayMode(RadiographyPhotometricDisplayMode  prefferedPhotometricDisplayMode);

  private:
    void UpdateTransform();

    void AddToExtent(Extent2D& extent,
                     double x,
                     double y) const;

    void SetIndex(size_t index)
    {
      index_ = index;
    }

    bool Contains(double x,
                  double y) const;

    void DrawBorders(CairoContext& context,
                     double zoom);

  public:
    RadiographyLayer(const RadiographyScene& scene);

    virtual ~RadiographyLayer()
    {
    }

    virtual const AffineTransform2D& GetTransform() const
    {
      return transform_;
    }

    virtual const AffineTransform2D& GetTransformInverse() const
    {
      return transformInverse_;
    }

    size_t GetIndex() const
    {
      return index_;
    }

    const RadiographyScene& GetScene() const
    {
      return scene_;
    }

    const Geometry& GetGeometry() const
    {
      return geometry_;
    }

    void SetGeometry(const Geometry& geometry);

    void ResetCrop();

    void SetCrop(unsigned int x,       // those are pixel coordinates/size
                 unsigned int y,
                 unsigned int width,
                 unsigned int height);

    void SetCrop(const Extent2D& sceneExtent)
    {
      Extent2D imageCrop;

      {
        double x = sceneExtent.GetX1();
        double y = sceneExtent.GetY1();
        GetTransformInverse().Apply(x, y);
        imageCrop.AddPoint(x, y);
      }

      {
        double x = sceneExtent.GetX2();
        double y = sceneExtent.GetY2();
        GetTransformInverse().Apply(x, y);
        imageCrop.AddPoint(x, y);
      }

      SetCrop(static_cast<unsigned int>(std::max(0.0, std::floor(imageCrop.GetX1()))),
              static_cast<unsigned int>(std::max(0.0, std::floor(imageCrop.GetY1()))),
              std::min(width_, static_cast<unsigned int>(std::ceil(imageCrop.GetWidth()))),
              std::min(height_, static_cast<unsigned int>(std::ceil(imageCrop.GetHeight())))
              );
    }


    void GetCrop(unsigned int& x,
                 unsigned int& y,
                 unsigned int& width,
                 unsigned int& height) const;

    void SetAngle(double angle);

    void SetPan(double x,
                double y);

    void SetFlipVertical(bool flip); //  mirrors image around an horizontal axis (note: flip is applied before the rotation !)

    void SetFlipHorizontal(bool flip); //  mirrors image around a vertical axis (note: flip is applied before the rotation !)

    void SetResizeable(bool resizeable)
    {
      geometry_.SetResizeable(resizeable);
    }

    void SetSize(unsigned int width,
                 unsigned int height,
                 bool emitLayerEditedEvent = true);

    bool HasSize() const
    {
      return hasSize_;
    }

    unsigned int GetWidth() const
    {
      return width_;
    }

    unsigned int GetHeight() const
    {
      return height_;
    }

    virtual Extent2D GetSceneExtent(bool minimal) const;

    virtual bool GetPixel(unsigned int& imageX,
                          unsigned int& imageY,
                          double sceneX,
                          double sceneY) const;

    void SetPixelSpacing(double x,
                         double y,
                         bool emitLayerEditedEvent = true);

    void GetCenter(double& centerX,
                   double& centerY) const;

    virtual void GetControlPoint(ControlPoint& cpScene /* out in scene coordinates */,
                                 size_t index) const;

    virtual size_t GetControlPointCount() const;

    bool LookupControlPoint(ControlPoint& cpScene /* out */,
                            double x,
                            double y,
                            double zoom,
                            double viewportDistance) const;

    virtual bool GetDefaultWindowing(float& center,
                                     float& width) const = 0;

    RadiographyPhotometricDisplayMode GetPreferredPhotomotricDisplayMode() const
    {
      return prefferedPhotometricDisplayMode_;
    }

    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation,
                        float windowCenter,
                        float windowWidth,
                        bool applyWindowing) const = 0;

    virtual bool GetRange(float& minValue,
                          float& maxValue) const = 0;

    virtual size_t GetApproximateMemoryUsage() const // this is used to limit the number of scenes loaded in RAM when resources are limited (we actually only count the size used by the images, not the C structs)
    {
      return 0;
    }
  };
}