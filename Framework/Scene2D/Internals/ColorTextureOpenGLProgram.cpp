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


#include "ColorTextureOpenGLProgram.h"

namespace OrthancStone
{
  namespace Internals
  {
    ColorTextureOpenGLProgram::ColorTextureOpenGLProgram(OpenGL::IOpenGLContext&  context) :
      context_(context)
    {
      static const char* VERTEX_SHADER = 
        "attribute vec2 a_texcoord;    \n"
        "attribute vec4 a_position;    \n"
        "uniform mat4 u_matrix;        \n"
        "varying vec2 v_texcoord;      \n"
        "void main()                   \n"
        "{                             \n"
        "  gl_Position = u_matrix * a_position; \n"
        "  v_texcoord = a_texcoord;             \n"
        "}";

      static const char* FRAGMENT_SHADER = 
        "uniform sampler2D u_texture;  \n"
        "varying vec2 v_texcoord;      \n"
        "void main()                   \n"
        "{                             \n"
        "  gl_FragColor = texture2D(u_texture, v_texcoord); \n"
        "}";

      static const float POSITIONS[COMPONENTS * COUNT] = {
        0, 0,
        0, 1,
        1, 0,
        1, 0,
        0, 1,
        1, 1
      };
        
      context_.MakeCurrent();

      program_.reset(new OpenGL::OpenGLProgram);
      program_->CompileShaders(VERTEX_SHADER, FRAGMENT_SHADER);

      positionLocation_ = program_->GetAttributeLocation("a_position");
      textureLocation_ = program_->GetAttributeLocation("a_texcoord");

      glGenBuffers(2, buffers_);

      glBindBuffer(GL_ARRAY_BUFFER, buffers_[0]);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * COMPONENTS * COUNT, POSITIONS, GL_STATIC_DRAW);

      glBindBuffer(GL_ARRAY_BUFFER, buffers_[1]);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * COMPONENTS * COUNT, POSITIONS, GL_STATIC_DRAW);
    }

      
    ColorTextureOpenGLProgram::~ColorTextureOpenGLProgram()
    {
      context_.MakeCurrent();
      glDeleteBuffers(2, buffers_);
    }

    
    void ColorTextureOpenGLProgram::Apply(OpenGL::OpenGLTexture& texture,
                                          const AffineTransform2D& transform,
                                          bool useAlpha)
    {
      context_.MakeCurrent();
      program_->Use();

      AffineTransform2D scale = AffineTransform2D::CreateScaling
        (texture.GetWidth(), texture.GetHeight());

      AffineTransform2D t = AffineTransform2D::Combine(transform, scale);

      float m[16];
      t.ConvertToOpenGLMatrix(m, context_.GetCanvasWidth(), context_.GetCanvasHeight());

      texture.Bind(program_->GetUniformLocation("u_texture"));
      glUniformMatrix4fv(program_->GetUniformLocation("u_matrix"), 1, GL_FALSE, m);

      glBindBuffer(GL_ARRAY_BUFFER, buffers_[0]);
      glEnableVertexAttribArray(positionLocation_);
      glVertexAttribPointer(positionLocation_, COMPONENTS, GL_FLOAT, GL_FALSE, 0, 0);

      glBindBuffer(GL_ARRAY_BUFFER, buffers_[1]);
      glEnableVertexAttribArray(textureLocation_);
      glVertexAttribPointer(textureLocation_, COMPONENTS, GL_FLOAT, GL_FALSE, 0, 0);

      if (useAlpha)
      {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLES, 0, COUNT);
        glDisable(GL_BLEND);
      }
      else
      {
        glDrawArrays(GL_TRIANGLES, 0, COUNT);
      }

      glDisableVertexAttribArray(positionLocation_);
      glDisableVertexAttribArray(textureLocation_);
    }
  }
}
