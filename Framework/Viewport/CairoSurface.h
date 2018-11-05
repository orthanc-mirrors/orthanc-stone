/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include <Core/Images/ImageAccessor.h>

#include <boost/noncopyable.hpp>
#include <cairo.h>

namespace OrthancStone
{
  class CairoSurface : public boost::noncopyable
  {
  private:
    cairo_surface_t* surface_;
    unsigned int     width_;
    unsigned int     height_;
    unsigned int     pitch_;
    void*            buffer_;

    void Release();

    void Allocate(unsigned int width,
                  unsigned int height);

  public:
    CairoSurface() :
      surface_(NULL)
    {
      Allocate(0, 0);
    }

    CairoSurface(unsigned int width,
                 unsigned int height) :
      surface_(NULL)
    {
      Allocate(width, height);
    }

    CairoSurface(Orthanc::ImageAccessor& accessor);

    ~CairoSurface()
    {
      Release();
    }

    void SetSize(unsigned int width,
                 unsigned int height);

    void Copy(const CairoSurface& other);

    unsigned int GetWidth() const
    {
      return width_;
    }

    unsigned int GetHeight() const
    {
      return height_;
    }

    unsigned int GetPitch() const
    {
      return pitch_;
    }

    const void* GetBuffer() const
    {
      return buffer_;
    }

    void* GetBuffer()
    {
      return buffer_;
    }

    cairo_surface_t* GetObject()
    {
      return surface_;
    }

    void GetReadOnlyAccessor(Orthanc::ImageAccessor& target) const;

    void GetWriteableAccessor(Orthanc::ImageAccessor& target);
  };
}
