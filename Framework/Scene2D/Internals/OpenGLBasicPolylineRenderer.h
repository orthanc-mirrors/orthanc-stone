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
#include "../PolylineSceneLayer.h"
#include "CompositorHelper.h"

namespace OrthancStone
{
  namespace Internals
  {
    class OpenGLBasicPolylineRenderer : public CompositorHelper::ILayerRenderer
    {
    private:
      OpenGL::IOpenGLContext&  context_;
      PolylineSceneLayer       layer_;

    public:
      OpenGLBasicPolylineRenderer(OpenGL::IOpenGLContext& context,
                                  const PolylineSceneLayer& layer);

      virtual void Render(const AffineTransform2D& transform);

      virtual void Update(const ISceneLayer& layer);
    };
  }
}