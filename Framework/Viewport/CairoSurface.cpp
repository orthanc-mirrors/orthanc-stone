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


#include "CairoSurface.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/ImageProcessing.h>

namespace OrthancStone
{
  void CairoSurface::Release()
  {
    if (surface_)
    {
      cairo_surface_destroy(surface_);
      surface_ = NULL;
    }
  }


  void CairoSurface::Allocate(unsigned int width,
                              unsigned int height)
  {
    Release();

    surface_ = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    if (!surface_)
    {
      // Should never occur
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    if (cairo_surface_status(surface_) != CAIRO_STATUS_SUCCESS)
    {
      LOG(ERROR) << "Cannot create a Cairo surface";
      cairo_surface_destroy(surface_);
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    width_ = width;
    height_ = height;
    pitch_ = cairo_image_surface_get_stride(surface_);
    buffer_ = cairo_image_surface_get_data(surface_);
  }


  CairoSurface::CairoSurface(Orthanc::ImageAccessor& accessor)
  {
    if (accessor.GetFormat() != Orthanc::PixelFormat_BGRA32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }      

    width_ = accessor.GetWidth();
    height_ = accessor.GetHeight();
    pitch_ = accessor.GetPitch();
    buffer_ = accessor.GetBuffer();

    surface_ = cairo_image_surface_create_for_data
      (reinterpret_cast<unsigned char*>(buffer_), CAIRO_FORMAT_RGB24, width_, height_, pitch_);
    if (!surface_)
    {
      // Should never occur
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    if (cairo_surface_status(surface_) != CAIRO_STATUS_SUCCESS)
    {
      LOG(ERROR) << "Bad pitch for a Cairo surface";
      cairo_surface_destroy(surface_);
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  void CairoSurface::SetSize(unsigned int width,
                             unsigned int height)
  {
    if (width_ != width ||
        height_ != height)
    {
      Allocate(width, height);
    }
  }


  void CairoSurface::Copy(const CairoSurface& other)
  {
    Orthanc::ImageAccessor source = other.GetConstAccessor();
    Orthanc::ImageAccessor target = GetAccessor();
    Orthanc::ImageProcessing::Copy(target, source);
  }


  Orthanc::ImageAccessor CairoSurface::GetConstAccessor() const
  {
    Orthanc::ImageAccessor accessor;
    accessor.AssignReadOnly(Orthanc::PixelFormat_BGRA32, width_, height_, pitch_, buffer_);
    return accessor;
  }


  Orthanc::ImageAccessor CairoSurface::GetAccessor()
  {
    Orthanc::ImageAccessor accessor;
    accessor.AssignWritable(Orthanc::PixelFormat_BGRA32, width_, height_, pitch_, buffer_);
    return accessor;
  }
}
