/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include "OpenGLTextureArray.h"

#if defined(__EMSCRIPTEN__)
#  if !defined(ORTHANC_WEBGL2_HEAP_COMPAT)
#    error The macro ORTHANC_WEBGL2_HEAP_COMPAT must be defined
#  endif
#endif

#include "OpenGLFramebuffer.h"
#include "OpenGLTexture.h"

#include <Images/Image.h>
#include <OrthancException.h>

#include <boost/lexical_cast.hpp>
#include <cassert>

namespace OrthancStone
{
  namespace OpenGL
  {
    OpenGLTextureArray::OpenGLTextureArray(IOpenGLContext& context) :
      context_(context),
      texture_(0),
      width_(0),
      height_(0),
      depth_(0),
      format_(Orthanc::PixelFormat_Float32),
      isLinearInterpolation_(false)
    {
      if (context.IsContextLost())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "OpenGL context has been lost");
      }

      glGenTextures(1, &texture_);
      ORTHANC_OPENGL_CHECK("glGenTextures()");

      if (texture_ == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Cannot create an OpenGL texture array");
      }
    }


    OpenGLTextureArray::~OpenGLTextureArray()
    {
      assert(texture_ != 0);
      glDeleteTextures(1, &texture_);
    }


    void OpenGLTextureArray::Setup(Orthanc::PixelFormat format,
                                   unsigned int width,
                                   unsigned int height,
                                   unsigned int depth,
                                   bool isLinearInterpolation)
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);
      ORTHANC_OPENGL_CHECK("glBindTexture(GL_TEXTURE_2D_ARRAY)");

      GLenum sourceFormat, internalFormat, pixelType;
      OpenGLTexture::ConvertToOpenGLFormats(sourceFormat, internalFormat, pixelType, format);

      glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, depth,
                   0, sourceFormat, pixelType, NULL);
      ORTHANC_OPENGL_CHECK("glTexImage3D()");

#if !defined(__EMSCRIPTEN__)
      /**
       * glGetTexLevelParameteriv() was introduced in OpenGL ES 3.1,
       * but WebGL 2 only supports OpenGL ES 3.0, so it is not
       * available in WebAssembly:
       * https://registry.khronos.org/OpenGL-Refpages/es3.1/html/glGetTexLevelParameter.xhtml
       **/
      GLint w, h, d;
      glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH, &w);
      glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_HEIGHT, &h);
      glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_DEPTH, &d);
      if (width != static_cast<unsigned int>(w) ||
          height != static_cast<unsigned int>(h) ||
          depth != static_cast<unsigned int>(d))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Your GPU cannot create an array of textures of size " +
                                        boost::lexical_cast<std::string>(width) + " x " +
                                        boost::lexical_cast<std::string>(height) + " x " +
                                        boost::lexical_cast<std::string>(depth));
      }
#endif

      format_ = format;
      width_ = width;
      height_ = height;
      depth_ = depth;
      isLinearInterpolation_ = isLinearInterpolation;

      GLint interpolation = (isLinearInterpolation ? GL_LINEAR : GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, interpolation);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, interpolation);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }


    void OpenGLTextureArray::SetClampingToZero()
    {
#if defined(__EMSCRIPTEN__)
      /**
       * This is because WebGL 2 derives from OpenGL ES 3.0, which
       * doesn't support GL_CLAMP_TO_BORDER, as can be seen here:
       * https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glTexParameter.xhtml
       **/
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                      "OpenGLTextureArray::SetClampingToZero() is not available in WebGL 2");
#else
      ORTHANC_OPENGL_CHECK("Entering OpenGLTextureArray::SetClampingToZero()");

      glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

      GLfloat colorfv[4] = { 0, 0, 0, 0 };
      glTexParameterfv(texture_, GL_TEXTURE_BORDER_COLOR, colorfv);

      ORTHANC_OPENGL_CHECK("Exiting OpenGLTextureArray::SetClampingToZero()");
#endif
    }


    void OpenGLTextureArray::Bind(GLint location) const
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);
      glUniform1i(location, 0 /* texture unit */);
    }


    void OpenGLTextureArray::BindAsTextureUnit(GLint location,
                                               unsigned int unit) const
    {
      if (unit >= 32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      assert(GL_TEXTURE0 + 1 == GL_TEXTURE1 &&
             GL_TEXTURE0 + 31 == GL_TEXTURE31);

      glActiveTexture(GL_TEXTURE0 + unit);
      glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);
      glUniform1i(location, unit /* texture unit */);
    }


    void OpenGLTextureArray::Upload(const Orthanc::ImageAccessor& image,
                                    unsigned int layer)
    {
      if (image.GetWidth() != width_ ||
          image.GetHeight() != height_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
      }

      if (layer >= depth_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      if (width_ != 0 &&
          height_ != 0)
      {
        GLenum sourceFormat, internalFormat, pixelType;
        OpenGLTexture::ConvertToOpenGLFormats(sourceFormat, internalFormat, pixelType, image.GetFormat());

        glBindTexture(GL_TEXTURE_2D_ARRAY, texture_);

#if defined(__EMSCRIPTEN__) && (ORTHANC_WEBGL2_HEAP_COMPAT == 1)
        // Check out "OpenGLTexture.cpp" for an explanation
        EM_ASM({
            var ptr = emscriptenWebGLGetTexPixelData($5, $4, $2, $3, $0, $1);
            GLctx.texSubImage3D(GLctx.TEXTURE_2D_ARRAY, 0, 0 /* x offset */, 0 /* y offset */,
                                $6, $2, $3, 1 /* depth */, $4, $5, ptr);
          },
          image.GetConstBuffer(),  // $0
          internalFormat,          // $1
          image.GetWidth(),        // $2
          image.GetHeight(),       // $3
          sourceFormat,            // $4
          pixelType,               // $5
          layer);                  // $6
#else
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0 /* x offset */, 0 /* y offset */, layer /* z offset */,
                        width_, height_, 1 /* depth */, sourceFormat, pixelType, image.GetConstBuffer());
#endif
      }
    }


    size_t OpenGLTextureArray::GetMemoryBufferSize() const
    {
      return static_cast<size_t>(Orthanc::GetBytesPerPixel(format_)) * width_ * height_ * depth_;
    }


    void OpenGLTextureArray::Download(void* targetBuffer,
                                      size_t targetSize) const
    {
      if (targetSize != GetMemoryBufferSize())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      else if (targetSize == 0)
      {
        return;
      }
      else if (targetBuffer == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      else
      {
#if 1 || defined(__EMSCRIPTEN__)
        /**
         * The "glGetTexImage()" function is unavailable in WebGL, it
         * is necessary to use a framebuffer:
         * https://stackoverflow.com/a/15064957
         **/
        OpenGLFramebuffer framebuffer(context_);

        const size_t sliceSize = targetSize / depth_;

        Orthanc::Image tmp(GetFormat(), GetWidth(), GetHeight(), true);
        if (sliceSize != tmp.GetPitch() * height_)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        for (unsigned int layer = 0; layer < depth_; layer++)
        {
          framebuffer.ReadTexture(tmp, *this, layer);
          memcpy(reinterpret_cast<uint8_t*>(targetBuffer) + layer * sliceSize, tmp.GetBuffer(), sliceSize);
        }

#else
        glBindTexture(GL_TEXTURE_3D, texture_);

        switch (format_)
        {
          case Orthanc::PixelFormat_Grayscale8:
            glGetTexImage(GL_TEXTURE_3D, 0 /* base level */, GL_RED, GL_UNSIGNED_BYTE, targetBuffer);
            break;

          case Orthanc::PixelFormat_RGB24:
            glGetTexImage(GL_TEXTURE_3D, 0 /* base level */, GL_RGB, GL_UNSIGNED_BYTE, targetBuffer);
            break;

          case Orthanc::PixelFormat_RGBA32:
            glGetTexImage(GL_TEXTURE_3D, 0 /* base level */, GL_RGBA, GL_UNSIGNED_BYTE, targetBuffer);
            break;

          case Orthanc::PixelFormat_Float32:
            glGetTexImage(GL_TEXTURE_3D, 0 /* base level */, GL_RED, GL_FLOAT, targetBuffer);
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }
#endif
      }
    }


    void OpenGLTextureArray::Download(std::string& target) const
    {
      target.resize(GetMemoryBufferSize());
      Download(target);
    }
  }
}
