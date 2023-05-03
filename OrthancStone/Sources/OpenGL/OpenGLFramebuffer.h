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
    class OpenGLTexture;
    class OpenGLTextureArray;

    class OpenGLFramebuffer : public boost::noncopyable
    {
    private:
      IOpenGLContext&  context_;
      GLuint           framebuffer_;

      void SetupTextureTarget();

      void ReadContent(Orthanc::ImageAccessor& target);

    public:
      OpenGLFramebuffer(IOpenGLContext& context);

      ~OpenGLFramebuffer();

      void SetTarget(OpenGLTexture& target);

      void SetTarget(OpenGLTextureArray& target,
                     unsigned int layer);

      void ReadTexture(Orthanc::ImageAccessor& target,
                       const OpenGLTexture& source);

      void ReadTexture(Orthanc::ImageAccessor& target,
                       const OpenGLTextureArray& source,
                       unsigned int layer);
    };
  }
}
