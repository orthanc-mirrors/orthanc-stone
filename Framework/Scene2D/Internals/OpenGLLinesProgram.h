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
#include "../../Toolbox/AffineTransform2D.h"
#include "../PolylineSceneLayer.h"

namespace OrthancStone
{
  namespace Internals
  {
    class OpenGLLinesProgram : public boost::noncopyable
    {
    public:
      class Data : public boost::noncopyable
      {
      private:
        class Segment;
        
        OpenGL::IOpenGLContext&  context_;
        GLuint                   buffers_[2];
        size_t                   verticesCount_;
        float                    thickness_;
        float                    red_;
        float                    green_;
        float                    blue_;

      public:
        Data(OpenGL::IOpenGLContext& context,
             const PolylineSceneLayer& layer);
        
        ~Data();

        bool IsEmpty() const
        {
          return verticesCount_ == 0;
        }

        const size_t GetVerticesCount() const
        {
          return verticesCount_;
        }

        GLuint GetVerticesBuffer() const;

        GLuint GetMiterDirectionsBuffer() const;

        float GetThickness() const
        {
          return thickness_;
        }

        float GetRed() const
        {
          return red_;
        }

        float GetGreen() const
        {
          return green_;
        }

        float GetBlue() const
        {
          return blue_;
        }
      };
      
    private:
      OpenGL::IOpenGLContext&               context_;
      std::auto_ptr<OpenGL::OpenGLProgram>  program_;

    public:
      OpenGLLinesProgram(OpenGL::IOpenGLContext&  context);

      void Apply(const Data& data,
                 const AffineTransform2D& transform,
                 bool antialiasing,
                 bool scaleIndependantThickness);
    };
  }
}