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

#if !defined(ORTHANC_ENABLE_OPENGL)
#  error The macro ORTHANC_ENABLE_OPENGL must be defined
#endif

#if ORTHANC_ENABLE_OPENGL != 1
#  error Support for OpenGL is disabled
#endif

#if defined(__APPLE__)
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
#else
#  include <GL/gl.h>
#  include <GL/glext.h>
#endif

#include <Core/Images/ImageAccessor.h>

#include <boost/noncopyable.hpp>


namespace OrthancStone
{
  namespace OpenGL
  {
    class OpenGLTexture : public boost::noncopyable
    {
    private:
      GLuint        texture_;
      unsigned int  width_;
      unsigned int  height_;

    public:
      OpenGLTexture();

      ~OpenGLTexture();

      unsigned int GetWidth() const
      {
        return width_;
      }

      unsigned int GetHeight() const
      {
        return height_;
      }

      void Load(const Orthanc::ImageAccessor& image,
                bool isLinearInterpolation);

      void Bind(GLint location);
    };
  }
}
