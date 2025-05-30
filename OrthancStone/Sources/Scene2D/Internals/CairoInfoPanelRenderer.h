/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#pragma once

#include "../../Wrappers/CairoSurface.h"
#include "CompositorHelper.h"
#include "ICairoContextProvider.h"

namespace OrthancStone
{
  namespace Internals
  {
    class CairoInfoPanelRenderer : public CompositorHelper::ILayerRenderer
    {
    private:
      ICairoContextProvider& target_;
      CairoSurface           texture_;
      BitmapAnchor           anchor_;
      bool                   isLinearInterpolation_;
      bool                   applySceneRotation_;

    public:
      CairoInfoPanelRenderer(ICairoContextProvider& target,
                             const ISceneLayer& layer) :
        target_(target),
        anchor_(BitmapAnchor_TopLeft),
        applySceneRotation_(false)

      {
        Update(layer);
      }

      virtual void Update(const ISceneLayer& layer) ORTHANC_OVERRIDE;
      
      virtual void Render(const AffineTransform2D& transform,
                          unsigned int canvasWidth,
                          unsigned int canvasHeight) ORTHANC_OVERRIDE;
    };
  }
}
