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

#include "RadiographyLayer.h"

#include <Compatibility.h>

namespace OrthancStone
{
  class RadiographyScene;

  // creates a transparent layer whose alpha channel is provided as a UINT8 image to SetAlpha.
  // The color of the "mask" is either defined by a ForegroundValue or by the center value of the
  // windowing from the scene.
  class RadiographyAlphaLayer : public RadiographyLayer
  {
  private:
    std::unique_ptr<Orthanc::ImageAccessor>  alpha_;       // Grayscale8 in the range [0, 255]  0 = transparent, 255 = opaque -> the foreground value will be displayed
    float                                  foreground_;  // in the range [0.0, 65535.0]

  public:
    RadiographyAlphaLayer(const RadiographyScene& scene) :
      RadiographyLayer(scene),
      foreground_(0)
    {
    }


    void SetForegroundValue(float foreground)
    {
      foreground_ = foreground;
    }

    float GetForegroundValue() const
    {
      return foreground_;
    }

    void SetAlpha(Orthanc::ImageAccessor* image);

    virtual bool GetDefaultWindowing(float& center,
                                     float& width) const
    {
      return false;
    }


    virtual void Render(Orthanc::ImageAccessor& buffer,
                        const AffineTransform2D& viewTransform,
                        ImageInterpolation interpolation,
                        float windowCenter,
                        float windowWidth,
                        bool applyWindowing) const;

    virtual bool GetRange(float& minValue,
                          float& maxValue) const;

    const Orthanc::ImageAccessor& GetAlpha() const
    {
      return *(alpha_.get());
    }


  };
}
