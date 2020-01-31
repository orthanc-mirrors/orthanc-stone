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


#include "OpenGLInfoPanelRenderer.h"

namespace OrthancStone
{
  namespace Internals
  {
    void OpenGLInfoPanelRenderer::LoadTexture(const InfoPanelSceneLayer& layer)
    {
      if (!context_.IsContextLost())
      {
        context_.MakeCurrent();
        texture_.reset(new OpenGL::OpenGLTexture(context_));
        texture_->Load(layer.GetTexture(), layer.IsLinearInterpolation());
        anchor_ = layer.GetAnchor();
      }
    }

    OpenGLInfoPanelRenderer::OpenGLInfoPanelRenderer(OpenGL::IOpenGLContext& context,
                                                     OpenGLColorTextureProgram& program,
                                                     const InfoPanelSceneLayer& layer) :
      context_(context),
      program_(program),
      anchor_(BitmapAnchor_TopLeft)
    {
      LoadTexture(layer);
    }

    
    void OpenGLInfoPanelRenderer::Render(const AffineTransform2D& transform,
                                         unsigned int canvasWidth,
                                         unsigned int canvasHeight)
    {
      if (!context_.IsContextLost() && texture_.get() != NULL)
      {
        int dx = 0, dy = 0;
        InfoPanelSceneLayer::ComputeAnchorLocation(
          dx, dy, anchor_, texture_->GetWidth(), texture_->GetHeight(),
          canvasWidth, canvasHeight);

        // The position of this type of layer is layer: Ignore the
        // "transform" coming from the scene
        program_.Apply(*texture_, AffineTransform2D::CreateOffset(dx, dy), true);
      }
    }
  }
}
