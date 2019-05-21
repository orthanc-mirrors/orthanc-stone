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


#include "ColorFrameRenderer.h"

#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>

namespace Deprecated
{
  OrthancStone::CairoSurface* ColorFrameRenderer::GenerateDisplay(const RenderStyle& style)
  {
    std::auto_ptr<OrthancStone::CairoSurface> display
      (new OrthancStone::CairoSurface(frame_->GetWidth(), frame_->GetHeight(), false /* no alpha */));

    Orthanc::ImageAccessor target;
    display->GetWriteableAccessor(target);
    
    Orthanc::ImageProcessing::Convert(target, *frame_);

    return display.release();
  }


  ColorFrameRenderer::ColorFrameRenderer(const Orthanc::ImageAccessor& frame,
                                         const OrthancStone::CoordinateSystem3D& framePlane,
                                         double pixelSpacingX,
                                         double pixelSpacingY,
                                         bool isFullQuality) :
    FrameRenderer(framePlane, pixelSpacingX, pixelSpacingY, isFullQuality),
    frame_(Orthanc::Image::Clone(frame))
  {
    if (frame_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    if (frame_->GetFormat() != Orthanc::PixelFormat_RGB24)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }
  }
}
