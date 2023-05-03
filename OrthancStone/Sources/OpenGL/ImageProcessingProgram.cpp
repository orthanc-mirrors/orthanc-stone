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


#include "ImageProcessingProgram.h"

#include "OpenGLFramebuffer.h"

#include <OrthancException.h>


static const unsigned int DIMENSIONS = 2;  // Number of dimensions (we draw in 2D)
static const unsigned int VERTICES = 6;  // 2 triangles in 2D (each triangle has 3 vertices)


static const float TRIANGLES[DIMENSIONS * VERTICES] = {
  // First triangle
  -1.0f, -1.0f,
  1.0f,  -1.0f,
  -1.0f,  1.0f,
  // Second triangle
  -1.0f,  1.0f,
  1.0f,  -1.0f,
  1.0f,   1.0f
};


/**
 * "varying" indicates variables that are shader by the vertex shader
 * and the fragment shader. The reason for "v_position" is that
 * "a_position" (position in the target frame buffer) ranges from -1
 * to 1, whereas texture samplers range from 0 to 1.
 **/
static const char* VERTEX_SHADER =
  "in vec2 a_position;                        \n"
  "out vec2 v_position;                       \n"
  "void main() {                              \n"
  "  v_position = (a_position + 1.0) / 2.0;   \n"
  "  gl_Position = vec4(a_position, 0, 1.0);  \n"
  "}                                          \n";


namespace OrthancStone
{
  namespace OpenGL
  {
    void ImageProcessingProgram::SetupPosition()
    {
      glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * DIMENSIONS * VERTICES, TRIANGLES, GL_STATIC_DRAW);
      GLint positionLocation = program_.GetAttributeLocation("a_position");
      glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
      glEnableVertexAttribArray(positionLocation);
    }


    ImageProcessingProgram::ImageProcessingProgram(IOpenGLContext& context,
                                                   const std::string& fragmentShader) :
      program_(context),
      quad_vertexbuffer(0)
    {
      if (context.IsContextLost())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "OpenGL context has been lost");
      }

      context.MakeCurrent();

      std::string version;

#if ORTHANC_ENABLE_WASM == 1
      /**
       * "#version 300 es" corresponds to:
       * - OpenGL ES version 3.0: https://registry.khronos.org/OpenGL-Refpages/es3.0/
       * - WebGL version 2.0
       * - GLSL ES version 3.00.6
       * - Based on version GLSL version 3.0
       *
       * Explanation for "highp":
       * https://emscripetn.org/docs/optimizing/Optimizing-WebGL.html
       * https://webglfundamentals.org/webgl/lessons/webgl-qna-when-to-choose-highp--mediump--lowp-in-shaders.html
       **/
      version = ("#version 300 es\n"
                 "precision highp float;\n"
                 "precision highp sampler2D;\n"
                 "precision highp sampler2DArray;\n");
#else
      /**
       * "#version 130" corresponds to:
       * - OpenGL version 3.0
       * - GLSL version 1.30.10
       **/
      version = "#version 130\n";
#endif

      program_.CompileShaders(version + VERTEX_SHADER, version + fragmentShader);

      glGenBuffers(1, &quad_vertexbuffer);
      if (quad_vertexbuffer == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
                                        "Cannot create OpenGL buffer");
      }
    }


    ImageProcessingProgram::~ImageProcessingProgram()
    {
      glDeleteBuffers(1, &quad_vertexbuffer);
    }


    void ImageProcessingProgram::Use(OpenGLTexture& target,
                                     OpenGLFramebuffer& framebuffer,
                                     bool checkStatus)
    {
      program_.Use(checkStatus);
      framebuffer.SetTarget(target);
      SetupPosition();
    }


    void ImageProcessingProgram::Use(OpenGLTextureArray& target,
                                     unsigned int targetLayer,
                                     OpenGLFramebuffer& framebuffer,
                                     bool checkStatus)
    {
      program_.Use(checkStatus);
      framebuffer.SetTarget(target, targetLayer);
      SetupPosition();
    }


    void ImageProcessingProgram::Render()
    {
      glClearColor(0.0, 0.0, 0.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT);

#if 1
      glDrawArrays(GL_TRIANGLES, 0, VERTICES);
#else
      // Simpler, but not available in WebGL
      glBegin(GL_QUADS);
      glVertex2f(-1,  1); // vertex 1
      glVertex2f(-1, -1); // vertex 2
      glVertex2f( 1, -1); // vertex 3
      glVertex2f( 1,  1); // vertex 4
      glEnd();
#endif

      ORTHANC_OPENGL_CHECK("glDrawArrays()");
    }
  }
}
