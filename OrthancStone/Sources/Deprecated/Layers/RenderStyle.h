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

#include "../../StoneEnumerations.h"
#include "../../Volumes/ImageBuffer3D.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <EmbeddedResources.h>

#include <stdint.h>

namespace Deprecated
{
  struct RenderStyle
  {
    bool visible_;
    bool reverse_;
    OrthancStone::ImageWindowing windowing_;
    float alpha_;   // In [0,1]
    bool applyLut_;
    Orthanc::EmbeddedResources::FileResourceId  lut_;
    bool drawGrid_;
    uint8_t drawColor_[3];
    float customWindowCenter_;
    float customWindowWidth_;
    OrthancStone::ImageInterpolation interpolation_;
    unsigned int fontSize_;
    
    RenderStyle();

    void ComputeWindowing(float& targetCenter,
                          float& targetWidth,
                          float defaultCenter,
                          float defaultWidth) const;

    void SetColor(uint8_t red,
                  uint8_t green,
                  uint8_t blue);

    bool FitRange(const OrthancStone::ImageBuffer3D& image,
                  const DicomFrameConverter& converter);
  };
}