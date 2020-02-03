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


#pragma once

#include "ILayerRenderer.h"

#include "../Toolbox/Slice.h"

namespace Deprecated
{
  class FrameRenderer : public ILayerRenderer
  {
  private:
    OrthancStone::CoordinateSystem3D            framePlane_;
    double                        pixelSpacingX_;
    double                        pixelSpacingY_;
    RenderStyle                   style_;
    bool                          isFullQuality_;
    std::auto_ptr<OrthancStone::CairoSurface>   display_;

  protected:
    virtual OrthancStone::CairoSurface* GenerateDisplay(const RenderStyle& style) = 0;

  public:
    FrameRenderer(const OrthancStone::CoordinateSystem3D& framePlane,
                  double pixelSpacingX,
                  double pixelSpacingY,
                  bool isFullQuality);

    virtual bool RenderLayer(OrthancStone::CairoContext& context,
                             const ViewportGeometry& view);

    virtual const OrthancStone::CoordinateSystem3D& GetLayerPlane()
    {
      return framePlane_;
    }

    virtual void SetLayerStyle(const RenderStyle& style);

    virtual bool IsFullQuality() 
    {
      return isFullQuality_;
    }

    // TODO: Avoid cloning the "frame"
    static ILayerRenderer* CreateRenderer(const Orthanc::ImageAccessor& frame,
                                          const Deprecated::Slice& framePlane,
                                          bool isFullQuality);
  };
}
