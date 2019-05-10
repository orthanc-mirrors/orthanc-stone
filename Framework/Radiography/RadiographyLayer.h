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


#pragma once

#include "../Toolbox/AffineTransform2D.h"
#include "../Toolbox/Extent2D.h"
#include "../Viewport/CairoContext.h"
#include "../Messages/IMessage.h"
#include "../Messages/IObservable.h"

namespace OrthancStone
{
  class RadiographyScene;

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

    };

  private:
    size_t             index_;
    bool               hasSize_;
    unsigned int       width_;
    unsigned int       height_;
    AffineTransform2D  transform_;
    AffineTransform2D  transformInverse_;
    Geometry           geometry_;
    PhotometricDisplayMode  prefferedPhotometricDisplayMode_;
    const RadiographyScene&   scene_;

  protected:
    virtual const AffineTransform2D& GetTransform() const
    {
      return transform_;
    }

    virtual const AffineTransform2D& GetTransformInverse() const
    {
      return transformInverse_;
    }

    void SetPreferredPhotomotricDisplayMode(PhotometricDisplayMode  prefferedPhotometricDisplayMode);

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
    RadiographyLayer(MessageBroker& broker, const RadiographyScene& scene);

    virtual ~RadiographyLayer()
    {
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

    void SetCrop(unsigned int x,
                 unsigned int y,
                 unsigned int width,
                 unsigned int height);

    void GetCrop(unsigned int& x,
                 unsigned int& y,
                 unsigned int& width,
                 unsigned int& height) const;

    void SetAngle(double angle);

    void SetPan(double x,
                double y);

    void SetResizeable(bool resizeable)
    {
      geometry_.SetResizeable(resizeable);
    }

    void SetSize(unsigned int width,
                 unsigned int height);

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

    Extent2D GetExtent() const;

    virtual bool GetPixel(unsigned int& imageX,
                          unsigned int& imageY,
                          double sceneX,
                          double sceneY) const;

    void SetPixelSpacing(double x,
                         double y);

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

    PhotometricDisplayMode GetPreferredPhotomotricDisplayMode() const
    {
      return prefferedPhotometricDisplayMode_;
    }

    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation) const = 0;

    virtual bool GetRange(float& minValue,
                          float& maxValue) const = 0;

    friend class RadiographyMaskLayer; // because it needs to GetTransform on the dicomLayer it relates to
  };
}
