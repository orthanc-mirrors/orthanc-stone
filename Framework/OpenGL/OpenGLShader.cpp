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


#include "OpenGLShader.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  namespace OpenGL
  {
    static GLuint CompileShader(GLenum type,
                                const std::string& source) 
    {
      // Create shader object
      const GLchar* sourceString[1];
      GLint sourceStringLengths[1];

      sourceString[0] = source.c_str();
      sourceStringLengths[0] = source.length();
      GLuint shader = glCreateShader(type);

      if (shader == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Cannot create an OpenGL shader");
      }
      else
      {
        // Assign and compile the source to the shader object
        glShaderSource(shader, 1, sourceString, sourceStringLengths);
        glCompileShader(shader);

        // Check if there were errors
        int infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 0)
        {
          std::string infoLog;
          infoLog.resize(infoLen + 1);
          glGetShaderInfoLog(shader, infoLen, NULL, &infoLog[0]);
          glDeleteShader(shader);

          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                          "Error while creating an OpenGL shader: " + infoLog);
        }
        else
        {
          return shader;
        }
      }
    }


    OpenGLShader::OpenGLShader(GLenum type,
                               const std::string& source)
    {
      shader_ = CompileShader(type, source);
      isValid_ = true;
    }

    
    OpenGLShader::~OpenGLShader()
    {
      if (isValid_)
      {
        glDeleteShader(shader_);
      }
    }


    GLuint OpenGLShader::Release()
    {
      if (isValid_)
      {
        isValid_ = false;
        return shader_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
  }
}
