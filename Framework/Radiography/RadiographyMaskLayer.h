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


#pragma once

#include "RadiographyLayer.h"
#include "Core/Images/Image.h"

namespace OrthancStone
{
  class RadiographyScene;
  class RadiographyDicomLayer;

  struct MaskPoint
  {
    unsigned int x;
    unsigned int y;

    MaskPoint(unsigned int x, unsigned int y)
      : x(x),
        y(y)
    {}
  };

  class RadiographyMaskLayer : public RadiographyLayer
  {
  private:
    std::vector<MaskPoint>            corners_;
    const RadiographyDicomLayer&      dicomLayer_;
    mutable bool                      invalidated_;
    float                             foreground_;

    mutable std::auto_ptr<Orthanc::ImageAccessor>  mask_;
  public:
    RadiographyMaskLayer(const RadiographyScene& scene, const RadiographyDicomLayer& dicomLayer,
                         float foreground) :
      RadiographyLayer(),
      dicomLayer_(dicomLayer),
      invalidated_(true),
      foreground_(foreground)
    {
    }

    void SetCorners(const std::vector<MaskPoint>& corners);

    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation) const;

    virtual size_t GetControlPointCount() const
    {
      return corners_.size();
    }

    virtual void GetControlPointInternal(ControlPoint& controlPoint,
                                         size_t index) const
    {
      controlPoint = ControlPoint(corners_[index].x, corners_[index].y, index);
    }

    virtual bool GetDefaultWindowing(float& center,
                                     float& width) const
    {
      return false;
    }

    virtual bool GetRange(float& minValue,
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

  protected:
    virtual const AffineTransform2D& GetTransform() const;

    virtual const AffineTransform2D& GetTransformInverse() const;


  private:
    void DrawMask() const;
    void DrawLine(const MaskPoint& start, const MaskPoint& end) const;

  };
}