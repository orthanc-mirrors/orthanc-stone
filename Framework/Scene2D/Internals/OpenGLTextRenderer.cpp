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


#include "OpenGLTextRenderer.h"

namespace OrthancStone
{
  namespace Internals
  {
    void OpenGLTextRenderer::LoadLayer(const TextSceneLayer& layer)
    {
      data_.reset(new OpenGLTextProgram::Data(context_, alphabet_, layer));
    }


    OpenGLTextRenderer::OpenGLTextRenderer(OpenGL::IOpenGLContext& context,
                                           OpenGLTextProgram& program,
                                           const GlyphTextureAlphabet& alphabet,
                                           OpenGL::OpenGLTexture& texture,
                                           const TextSceneLayer& layer) :
      context_(context),
      program_(program),
      alphabet_(alphabet),
      texture_(texture)
    {
      LoadLayer(layer);
    }

      
    void OpenGLTextRenderer::Render(const AffineTransform2D& transform)
    {
      if (data_.get() != NULL)
      {
        program_.Apply(texture_, *data_, transform);
      }
    }

      
    void OpenGLTextRenderer::Update(const ISceneLayer& layer)
    {
      LoadLayer(dynamic_cast<const TextSceneLayer&>(layer));
    }
  }
}