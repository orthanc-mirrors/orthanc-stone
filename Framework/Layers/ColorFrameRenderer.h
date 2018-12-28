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

#include "FrameRenderer.h"

namespace OrthancStone
{
  class ColorFrameRenderer : public FrameRenderer
  {
  private:
    std::auto_ptr<Orthanc::ImageAccessor>   frame_;  // In RGB24

  protected:
    virtual CairoSurface* GenerateDisplay(const RenderStyle& style);

  public:
    ColorFrameRenderer(const Orthanc::ImageAccessor& frame,
                       const CoordinateSystem3D& framePlane,
                       double pixelSpacingX,
                       double pixelSpacingY,
                       bool isFullQuality);
  };
}
