/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2024 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#pragma once

#include "OpenGLIncludes.h"
#include "IOpenGLContext.h"

#include <Images/ImageAccessor.h>

#include <boost/noncopyable.hpp>


namespace OrthancStone
{
  namespace OpenGL
  {
    class OpenGLTexture : public boost::noncopyable
    {
      friend class OpenGLFramebuffer;

    private:
      OpenGL::IOpenGLContext& context_;
      GLuint                  texture_;
      unsigned int            width_;
      unsigned int            height_;
      Orthanc::PixelFormat    format_;
      bool                    isLinearInterpolation_;

      void Setup(Orthanc::PixelFormat format,
                 unsigned int width,
                 unsigned int height,
                 bool isLinearInterpolation,
                 const void* data);

      /**
       * Returns the low-level OpenGL handle of the texture. Beware to
       * never change the size of the texture using this handle!
       **/
      GLuint GetId() const
      {
        return texture_;
      }

    public:
      explicit OpenGLTexture(OpenGL::IOpenGLContext& context);

      ~OpenGLTexture();

      Orthanc::PixelFormat GetFormat() const
      {
        return format_;
      }

      unsigned int GetWidth() const
      {
        return width_;
      }

      unsigned int GetHeight() const
      {
        return height_;
      }

      bool IsLinearInterpolation() const
      {
        return isLinearInterpolation_;
      }

      void Setup(Orthanc::PixelFormat format,
                 unsigned int width,
                 unsigned int height,
                 bool isLinearInterpolation)
      {
        Setup(format, width, height, isLinearInterpolation, NULL);
      }

      void Load(const Orthanc::ImageAccessor& image,
                bool isLinearInterpolation);

      void Bind(GLint location) const;

      void BindAsTextureUnit(GLint location,
                             unsigned int unit) const;

      Orthanc::ImageAccessor* Download(Orthanc::PixelFormat format) const;

      /**
       * By default, textures are mirrored at the borders. This
       * function will set out-of-image access to zero.
       **/
      void SetClampingToZero();

      static void ConvertToOpenGLFormats(GLenum& sourceFormat,
                                         GLenum& internalFormat,
                                         GLenum& pixelType,
                                         Orthanc::PixelFormat format);
    };
  }
}
