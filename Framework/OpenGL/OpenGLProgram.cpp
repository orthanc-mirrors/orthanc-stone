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


#include "OpenGLProgram.h"

#include "OpenGLShader.h"

#include <Core/OrthancException.h>


namespace OrthancStone
{
  namespace OpenGL
  {
    OpenGLProgram::OpenGLProgram()
    {
      program_ = glCreateProgram();
      if (program_ == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Cannot create an OpenGL program");
      }
    }


    OpenGLProgram::~OpenGLProgram()
    {
      assert(program_ != 0);
      glDeleteProgram(program_);
    }


    void OpenGLProgram::Use()
    {
      glUseProgram(program_);
    }

    
    void OpenGLProgram::CompileShaders(const std::string& vertexCode,
                                       const std::string& fragmentCode)
    {
      assert(program_ != 0);

      OpenGLShader vertexShader(GL_VERTEX_SHADER, vertexCode);
      OpenGLShader fragmentShader(GL_FRAGMENT_SHADER, fragmentCode);

      glAttachShader(program_, vertexShader.Release());
      glAttachShader(program_, fragmentShader.Release());
      glLinkProgram(program_);
      glValidateProgram(program_);
    }


    GLint OpenGLProgram::GetUniformLocation(const std::string& name)
    { 
      GLint location = glGetUniformLocation(program_, name.c_str());

      if (location == -1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem,
                                        "Inexistent uniform variable in shader: " + name);
      }
      else
      {
        return location;
      }
    }

    
    GLint OpenGLProgram::GetAttributeLocation(const std::string& name)
    { 
      GLint location = glGetAttribLocation(program_, name.c_str());

      if (location == -1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem,
                                        "Inexistent attribute in shader: " + name);
      }
      else
      {
        return location;
      }
    }
  }
}
