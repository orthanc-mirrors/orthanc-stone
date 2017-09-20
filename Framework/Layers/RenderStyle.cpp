/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include <Core/OrthancException.h>

namespace OrthancStone
{
  RenderStyle::RenderStyle()
  {
    visible_ = true;
    reverse_ = false;
    windowing_ = ImageWindowing_Default;
    alpha_ = 1;
    applyLut_ = false;
    lut_ = Orthanc::EmbeddedResources::COLORMAP_HOT;
    drawGrid_ = false;
    drawColor_[0] = 255;
    drawColor_[1] = 255;
    drawColor_[2] = 255;
    customWindowCenter_ = 128;
    customWindowWidth_ = 256;
    interpolation_ = ImageInterpolation_Nearest;
    fontSize_ = 14;
  }


  void RenderStyle::ComputeWindowing(float& targetCenter,
                                     float& targetWidth,
                                     float defaultCenter,
                                     float defaultWidth) const
  {
    switch (windowing_)
    {
      case ImageWindowing_Default:
        targetCenter = defaultCenter;
        targetWidth = defaultWidth;
        break;

      case ImageWindowing_Bone:
        targetCenter = 300;
        targetWidth = 2000;
        break;

      case ImageWindowing_Lung:
        targetCenter = -600;
        targetWidth = 1600;
        break;

      case ImageWindowing_Custom:
        targetCenter = customWindowCenter_;
        targetWidth = customWindowWidth_;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
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
}
