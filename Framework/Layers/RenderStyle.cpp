/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "RenderStyle.h"

#include "../Orthanc/Core/OrthancException.h"

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
