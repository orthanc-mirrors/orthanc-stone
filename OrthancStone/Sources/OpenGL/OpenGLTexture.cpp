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


#include "OpenGLTexture.h"
#include "IOpenGLContext.h"

#include <Images/Image.h>
#include <Logging.h>
#include <OrthancException.h>

namespace OrthancStone
{
  namespace OpenGL
  {
    OpenGLTexture::OpenGLTexture(OpenGL::IOpenGLContext& context) :
      texture_(0),
      width_(0),
      height_(0),
      context_(context)
    {
      if (!context_.IsContextLost())
      {
        // Generate a texture object
        glGenTextures(1, &texture_);
        if (texture_ == 0)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
            "Cannot create an OpenGL texture");
        }
      }
    }

    OpenGLTexture::~OpenGLTexture()
    {
      try
      {
        if (!context_.IsContextLost())
        {
          assert(texture_ != 0);
          ORTHANC_OPENGL_TRACE_CURRENT_CONTEXT("About to call glDeleteTextures");
          glDeleteTextures(1, &texture_);
        }
      }
      catch (const Orthanc::OrthancException& e)
      {
        if (e.HasDetails())
        {
          LOG(ERROR) << "OrthancException in ~OpenGLTexture: " << e.What() << " Details: " << e.GetDetails();
        }
        else
        {
          LOG(ERROR) << "OrthancException in ~OpenGLTexture: " << e.What();
        }
      }
      catch (const std::exception& e)
      {
        LOG(ERROR) << "std::exception in ~OpenGLTexture: " << e.what();
      }
      catch (...)
      {
        LOG(ERROR) << "Unknown exception in ~OpenGLTexture";
      }
    }

    void OpenGLTexture::Setup(Orthanc::PixelFormat format,
                              unsigned int width,
                              unsigned int height,
                              bool isLinearInterpolation,
                              const void* data)
    {
      if (context_.IsContextLost())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
            "OpenGL context has been lost");
      }
      else
      {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

        // Bind it
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_);

        GLenum sourceFormat, internalFormat, pixelType;
        ConvertToOpenGLFormats(sourceFormat, internalFormat, pixelType, format);

        width_ = width;
        height_ = height;

        GLint interpolation = (isLinearInterpolation ? GL_LINEAR : GL_NEAREST);

        // Load the texture from the image buffer
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height,
                     0, sourceFormat, pixelType, data);

#if !defined(__EMSCRIPTEN__)
        // "glGetTexLevelParameteriv()" seems to be undefined on WebGL
        GLint w, h;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
        if (width != static_cast<unsigned int>(w) ||
            height != static_cast<unsigned int>(h))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Your GPU cannot create a texture of size " +
                                          boost::lexical_cast<std::string>(width) + " x " +
                                          boost::lexical_cast<std::string>(height));
        }
#endif
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
    }

    void OpenGLTexture::Load(const Orthanc::ImageAccessor& image,
                             bool isLinearInterpolation)
    {
      if (image.GetPitch() != image.GetBytesPerPixel() * image.GetWidth())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                        "Pitch is not the same as the row size");
      }
      else
      {
        Setup(image.GetFormat(), image.GetWidth(), image.GetHeight(),
              isLinearInterpolation, image.GetConstBuffer());
      }
    }


    void OpenGLTexture::Bind(GLint location) const
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_);
      glUniform1i(location, 0 /* texture unit */);
    }


    void OpenGLTexture::BindAsTextureUnit(GLint location,
                                          unsigned int unit) const
    {
      if (unit >= 32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      
      assert(GL_TEXTURE0 + 1 == GL_TEXTURE1 &&
             GL_TEXTURE0 + 31 == GL_TEXTURE31);
      
      glActiveTexture(GL_TEXTURE0 + unit);
      glBindTexture(GL_TEXTURE_2D, texture_);
      glUniform1i(location, unit /* texture unit */);
    }


    Orthanc::ImageAccessor* OpenGLTexture::Download(Orthanc::PixelFormat format) const
    {
      if (context_.IsContextLost())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "OpenGL context is lost");
      }

      std::unique_ptr<Orthanc::ImageAccessor> target(new Orthanc::Image(format, width_, height_, true));
      assert(target->GetPitch() == width_ * Orthanc::GetBytesPerPixel(format));

      glBindTexture(GL_TEXTURE_2D, texture_);

      switch (format)
      {
        case Orthanc::PixelFormat_Grayscale8:
          glGetTexImage(GL_TEXTURE_2D, 0 /* base level */, GL_RED, GL_UNSIGNED_BYTE, target->GetBuffer());
          break;

        case Orthanc::PixelFormat_RGB24:
          glGetTexImage(GL_TEXTURE_2D, 0 /* base level */, GL_RGB, GL_UNSIGNED_BYTE, target->GetBuffer());
          break;

        case Orthanc::PixelFormat_RGBA32:
          glGetTexImage(GL_TEXTURE_2D, 0 /* base level */, GL_RGBA, GL_UNSIGNED_BYTE, target->GetBuffer());
          break;

        case Orthanc::PixelFormat_Float32:
          glGetTexImage(GL_TEXTURE_2D, 0 /* base level */, GL_RED, GL_FLOAT, target->GetBuffer());
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      return target.release();
    }
    

    void OpenGLTexture::SetClampingToZero()
    {
      glBindTexture(GL_TEXTURE_2D, texture_);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

      GLfloat colorfv[4] = { 0, 0, 0, 0 };
      glTextureParameterfv(texture_, GL_TEXTURE_BORDER_COLOR, colorfv);
    }


    void OpenGLTexture::ConvertToOpenGLFormats(GLenum& sourceFormat,
                                               GLenum& internalFormat,
                                               GLenum& pixelType,
                                               Orthanc::PixelFormat format)
    {
      switch (format)
      {
        case Orthanc::PixelFormat_Grayscale8:
          sourceFormat = GL_RED;
          internalFormat = GL_RED;
          pixelType = GL_UNSIGNED_BYTE;
          break;

        case Orthanc::PixelFormat_RGB24:
          sourceFormat = GL_RGB;
          internalFormat = GL_RGB;
          pixelType = GL_UNSIGNED_BYTE;
          break;

        case Orthanc::PixelFormat_RGBA32:
          sourceFormat = GL_RGBA;
          internalFormat = GL_RGBA;
          pixelType = GL_UNSIGNED_BYTE;
          break;

        case Orthanc::PixelFormat_Float32:
          sourceFormat = GL_RED;
          internalFormat = GL_R32F; // Don't use "GL_RED" here, as it clamps to [0,1]
          pixelType = GL_FLOAT;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                          "No support for this format in OpenGL textures: " +
                                          std::string(EnumerationToString(format)));
      }
    }
  }
}
