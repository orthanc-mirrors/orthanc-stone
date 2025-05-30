/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "DynamicBitmap.h"

#include <Images/Image.h>
#include <OrthancException.h>


namespace OrthancStone
{
  DynamicBitmap::DynamicBitmap(Orthanc::ImageAccessor* bitmap) :
    bitmap_(bitmap)
  {
    if (bitmap == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }


  DynamicBitmap::DynamicBitmap(const Orthanc::ImageAccessor& bitmap) :
    bitmap_(Orthanc::Image::Clone(bitmap))
  {
    if (bitmap_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  DynamicBitmap::DynamicBitmap(Orthanc::PixelFormat format,
                               unsigned int width,
                               unsigned int height,
                               bool forceMinimalPitch)
  {
    bitmap_.reset(new Orthanc::Image(format, width, height, forceMinimalPitch));
  }
}
