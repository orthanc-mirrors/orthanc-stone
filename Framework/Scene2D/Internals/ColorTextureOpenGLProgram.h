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


#pragma once

#include "../../OpenGL/IOpenGLContext.h"
#include "../../OpenGL/OpenGLProgram.h"
#include "../../OpenGL/OpenGLTexture.h"
#include "../../Toolbox/AffineTransform2D.h"

namespace OrthancStone
{
  namespace Internals
  {
    class ColorTextureOpenGLProgram : public boost::noncopyable
    {
    private:
      static const unsigned int COMPONENTS = 2;
      static const unsigned int COUNT = 6;  // 2 triangles in 2D

      OpenGL::IOpenGLContext&               context_;
      std::auto_ptr<OpenGL::OpenGLProgram>  program_;
      GLint                                 positionLocation_;
      GLint                                 textureLocation_;
      GLuint                                buffers_[2];

    public:
      ColorTextureOpenGLProgram(OpenGL::IOpenGLContext&  context);

      ~ColorTextureOpenGLProgram();

      void Apply(OpenGL::OpenGLTexture& texture,
                 const AffineTransform2D& transform,
                 bool useAlpha);
    };
  }
}
