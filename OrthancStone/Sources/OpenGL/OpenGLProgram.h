/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include "OpenGLIncludes.h"

#include <string>
#include <boost/noncopyable.hpp>

namespace OrthancStone
{
  namespace OpenGL
  {
    class IOpenGLContext;

    class OpenGLProgram : public boost::noncopyable
    {
    public:
      // WARNING: A global OpenGL context must be active to create this object!
      // the context is only passed so that it can be checked for loss
      // when destructing the program resource
      OpenGLProgram(OpenGL::IOpenGLContext& context);

      ~OpenGLProgram();

      void Use();

      // WARNING: A global OpenGL context must be active to run this method!
      void CompileShaders(const std::string& vertexCode,
                          const std::string& fragmentCode);

      GLint GetUniformLocation(const std::string& name);

      GLint GetAttributeLocation(const std::string& name);
    private:
      GLuint  program_;
      OpenGL::IOpenGLContext& context_;
    };
  }
}