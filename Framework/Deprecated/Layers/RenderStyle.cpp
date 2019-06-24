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


#include "RenderStyle.h"

#include "../../Volumes/ImageBuffer3D.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <Core/OrthancException.h>

namespace Deprecated
{
  RenderStyle::RenderStyle()
  {
    visible_ = true;
    reverse_ = false;
    windowing_ = OrthancStone::ImageWindowing_Custom;
    alpha_ = 1;
    applyLut_ = false;
    lut_ = Orthanc::EmbeddedResources::COLORMAP_HOT;
    drawGrid_ = false;
    drawColor_[0] = 255;
    drawColor_[1] = 255;
    drawColor_[2] = 255;
    customWindowCenter_ = 128;
    customWindowWidth_ = 256;
    interpolation_ = OrthancStone::ImageInterpolation_Nearest;
    fontSize_ = 14;
  }


  void RenderStyle::ComputeWindowing(float& targetCenter,
                                     float& targetWidth,
                                     float defaultCenter,
                                     float defaultWidth) const
  {
    if (windowing_ == OrthancStone::ImageWindowing_Custom)
    {
      targetCenter = customWindowCenter_;
      targetWidth = customWindowWidth_;
    }
    else
    {
      return ::OrthancStone::ComputeWindowing
        (targetCenter, targetWidth, windowing_, defaultCenter, defaultWidth);
    }
  }

  
  void RenderStyle::SetColor(uint8_t red,
                             uint8_t green,
                             uint8_t blue)
  {
    drawColor_[0] = red;
    drawColor_[1] = green;
    drawColor_[2] = blue;
  }


  bool RenderStyle::FitRange(const OrthancStone::ImageBuffer3D& image,
                             const DicomFrameConverter& converter)
  {
    float minValue, maxValue;

    windowing_ = OrthancStone::ImageWindowing_Custom;

    if (image.GetRange(minValue, maxValue))
    {  
      // casting the narrower type to wider before calling the + operator
      // will prevent overflowing (this is why the cast to double is only 
      // done on the first operand)
      customWindowCenter_ = static_cast<float>(
        converter.Apply((static_cast<double>(minValue) + maxValue) / 2.0));
      
      customWindowWidth_ = static_cast<float>(
        converter.Apply(static_cast<double>(maxValue) - minValue));
      
      if (customWindowWidth_ > 1)
      {
        return true;
      }
    }

    customWindowCenter_ = 128.0;
    customWindowWidth_ = 256.0;
    return false;
  }
}
