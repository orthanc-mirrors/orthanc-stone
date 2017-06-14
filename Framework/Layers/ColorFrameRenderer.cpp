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


#include "ColorFrameRenderer.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"

namespace OrthancStone
{
  CairoSurface* ColorFrameRenderer::GenerateDisplay(const RenderStyle& style)
  {
    std::auto_ptr<CairoSurface> display(new CairoSurface(frame_->GetWidth(), frame_->GetHeight()));

    Orthanc::ImageAccessor target = display->GetAccessor();
    Orthanc::ImageProcessing::Convert(target, *frame_);

    return display.release();
  }


  ColorFrameRenderer::ColorFrameRenderer(Orthanc::ImageAccessor* frame,
                                         const CoordinateSystem3D& frameSlice,
                                         double pixelSpacingX,
                                         double pixelSpacingY,
                                         bool isFullQuality) :
    FrameRenderer(frameSlice, pixelSpacingX, pixelSpacingY, isFullQuality),
    frame_(frame)
  {
    if (frame == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (frame_->GetFormat() != Orthanc::PixelFormat_RGB24)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }
  }
}
