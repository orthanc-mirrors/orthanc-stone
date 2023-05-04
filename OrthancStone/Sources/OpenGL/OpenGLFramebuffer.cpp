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


#include "OpenGLFramebuffer.h"

#if defined(__EMSCRIPTEN__)
#  if !defined(ORTHANC_WEBGL2_HEAP_COMPAT)
#    error The macro ORTHANC_WEBGL2_HEAP_COMPAT must be defined
#  endif
#endif

#include "OpenGLTexture.h"
#include "OpenGLTextureArray.h"

#include <OrthancException.h>


namespace OrthancStone
{
  namespace OpenGL
  {
    void OpenGLFramebuffer::SetupTextureTarget()
    {
      GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
      glDrawBuffers(1, drawBuffers);

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Incomplete setup of an OpenGL framebuffer");
      }
    }


    void OpenGLFramebuffer::ReadContent(Orthanc::ImageAccessor& target)
    {
      if (target.GetPitch() != target.GetWidth() * Orthanc::GetBytesPerPixel(target.GetFormat()) ||
          target.GetBuffer() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Image must have minimal pitch");
      }

      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
      {
        ORTHANC_OPENGL_CHECK("glCheckFramebufferStatus()");

        glViewport(0, 0, target.GetWidth(), target.GetHeight());

        GLenum sourceFormat, internalFormat, pixelType;
        OpenGLTexture::ConvertToOpenGLFormats(sourceFormat, internalFormat, pixelType, target.GetFormat());

#if defined(__EMSCRIPTEN__) && (ORTHANC_WEBGL2_HEAP_COMPAT == 1)
        // Check out "OpenGLTexture.cpp" for an explanation

        int framebufferFormat, framebufferType;
        glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &framebufferFormat);
        glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &framebufferType);

        switch (target.GetFormat())
        {
          case Orthanc::PixelFormat_RGBA32:
            if (sourceFormat != GL_RGBA ||
                internalFormat != GL_RGBA ||
                pixelType != GL_UNSIGNED_BYTE ||
                framebufferFormat != GL_RGBA ||
                framebufferType != GL_UNSIGNED_BYTE)
            {
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }

            EM_ASM({
                var ptr = emscriptenWebGLGetTexPixelData(GLctx.UNSIGNED_BYTE, GLctx.RGBA, $1, $2, $0, GLctx.RGBA);
                GLctx.readPixels(0, 0, $1, $2, GLctx.RGBA, GLctx.UNSIGNED_BYTE, ptr);
              },
              target.GetBuffer(),  // $0
              target.GetWidth(),   // $1
              target.GetHeight()); // $2
            break;

          case Orthanc::PixelFormat_Float32:
            // In Mozilla Firefox, "Float32" is not available as such. We
            // have to download an RGBA image in Float32.
            if (sourceFormat != GL_RED ||
                internalFormat != GL_R32F ||
                pixelType != GL_FLOAT ||
                framebufferType != GL_FLOAT)
            {
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }

            switch (framebufferFormat)
            {
              case GL_RGBA:
                // This is Mozilla Firefox
                EM_ASM({
                    var tmp = new Float32Array($1 * $2 * 4);
                    GLctx.readPixels(0, 0, $1, $2, GLctx.RGBA, GLctx.FLOAT, tmp);

                    // From RGBA to RED
                    var ptr = emscriptenWebGLGetTexPixelData(GLctx.FLOAT, GLctx.RED, $1, $2, $0, GLctx.R32F);
                    for (var i = 0; i < $1 * $2; i++) {
                      ptr[i] = tmp[4 * i];
                    }
                  },
                  target.GetBuffer(),  // $0
                  target.GetWidth(),   // $1
                  target.GetHeight()); // $2
                break;

              case GL_RED:
                // This is Chromium
                EM_ASM({
                    var ptr = emscriptenWebGLGetTexPixelData(GLctx.FLOAT, GLctx.RED, $1, $2, $0, GLctx.R32F);
                    GLctx.readPixels(0, 0, $1, $2, GLctx.RED, GLctx.FLOAT, ptr);
                  },
                  target.GetBuffer(),  // $0
                  target.GetWidth(),   // $1
                  target.GetHeight()); // $2
                break;

              default:
                throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
            }
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }
#else
        glReadPixels(0, 0, target.GetWidth(), target.GetHeight(), sourceFormat, pixelType, target.GetBuffer());
#endif

        ORTHANC_OPENGL_CHECK("glReadPixels()");
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Incomplete setup of an OpenGL framebuffer");
      }
    }


    OpenGLFramebuffer::OpenGLFramebuffer(IOpenGLContext& context) :
      context_(context),
      framebuffer_(0)
    {
      if (context.IsContextLost())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "OpenGL context has been lost");
      }

      glGenFramebuffers(1, &framebuffer_);
      if (framebuffer_ == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Cannot create an OpenGL framebuffer");
      }

      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    }


    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
      glDeleteFramebuffers(1, &framebuffer_);
    }


    void OpenGLFramebuffer::SetTarget(OpenGLTexture& target)
    {
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.GetId(), 0);
      ORTHANC_OPENGL_CHECK("glFramebufferTexture2D()");

      SetupTextureTarget();
      glViewport(0, 0, target.GetWidth(), target.GetHeight());
    }


    void OpenGLFramebuffer::SetTarget(OpenGLTextureArray& target,
                                      unsigned int layer)
    {
      if (layer >= target.GetDepth())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target.GetId(), 0, layer);
        ORTHANC_OPENGL_CHECK("glFramebufferTextureLayer()");

        SetupTextureTarget();
        glViewport(0, 0, target.GetWidth(), target.GetHeight());
      }
    }


    void OpenGLFramebuffer::ReadTexture(Orthanc::ImageAccessor& target,
                                        const OpenGLTexture& source)
    {
      if (target.GetWidth() != source.GetWidth() ||
          target.GetHeight() != source.GetHeight())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
      }
      else if (target.GetFormat() != source.GetFormat())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }
      else
      {
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source.GetId(), 0);
        ORTHANC_OPENGL_CHECK("glFramebufferTexture2D()");
        ReadContent(target);
      }
    }


    void OpenGLFramebuffer::ReadTexture(Orthanc::ImageAccessor& target,
                                        const OpenGLTextureArray& source,
                                        unsigned int layer)
    {
      if (target.GetWidth() != source.GetWidth() ||
          target.GetHeight() != source.GetHeight())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
      }
      else if (target.GetFormat() != source.GetFormat())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }
      else if (layer >= source.GetDepth())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else
      {
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, source.GetId(), 0, layer);
        ORTHANC_OPENGL_CHECK("glFramebufferTextureLayer()");
        ReadContent(target);
      }
    }
  }
}
