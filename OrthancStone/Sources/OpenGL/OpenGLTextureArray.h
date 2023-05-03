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


#pragma once

#include "OpenGLIncludes.h"
#include "IOpenGLContext.h"

#include <Images/ImageAccessor.h>

namespace OrthancStone
{
  namespace OpenGL
  {
    class OpenGLTextureArray : public boost::noncopyable
    {
      friend class OpenGLFramebuffer;

    private:
      OpenGL::IOpenGLContext& context_;
      GLuint                  texture_;
      unsigned int            width_;
      unsigned int            height_;
      unsigned int            depth_;
      Orthanc::PixelFormat    format_;
      bool                    isLinearInterpolation_;

      /**
       * Returns the low-level OpenGL handle of the texture
       * array. Beware to never change the size of the texture using
       * this handle!
       **/
      GLuint GetId() const
      {
        return texture_;
      }

    public:
      OpenGLTextureArray(IOpenGLContext& context);

      ~OpenGLTextureArray();

      unsigned int GetWidth() const
      {
        return width_;
      }

      unsigned int GetHeight() const
      {
        return height_;
      }

      unsigned int GetDepth() const
      {
        return depth_;
      }

      Orthanc::PixelFormat GetFormat() const
      {
        return format_;
      }

      bool IsLinearInterpolation() const
      {
        return isLinearInterpolation_;
      }

      void Setup(Orthanc::PixelFormat format,
                 unsigned int width,
                 unsigned int height,
                 unsigned int depth,
                 bool isLinearInterpolation);

      /**
       * By default, textures are mirrored at the borders. This
       * function will set out-of-image access to zero.
       **/
      void SetClampingToZero();

      void Bind(GLint location) const;

      void BindAsTextureUnit(GLint location,
                             unsigned int unit) const;

      void Upload(const Orthanc::ImageAccessor& image,
                  unsigned int layer);

      class DownloadedVolume : public boost::noncopyable
      {
      private:
        std::string           buffer_;
        Orthanc::PixelFormat  format_;
        unsigned int          width_;
        unsigned int          height_;
        unsigned int          depth_;

      public:
        DownloadedVolume(const OpenGLTextureArray& texture);

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

        unsigned int GetDepth() const
        {
          return depth_;
        }

        Orthanc::ImageAccessor* GetLayer(unsigned int layer) const;
      };
    };
  }
}
