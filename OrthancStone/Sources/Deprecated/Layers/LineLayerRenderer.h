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

namespace Deprecated
{
  class LineLayerRenderer : public ILayerRenderer
  {
  private:
    double              x1_;
    double              y1_;
    double              x2_;
    double              y2_;
    OrthancStone::CoordinateSystem3D  plane_;
    bool                visible_;
    uint8_t             color_[3];

  public:
    LineLayerRenderer(double x1,
                      double y1,
                      double x2,
                      double y2,
                      const OrthancStone::CoordinateSystem3D& plane);

    virtual bool RenderLayer(OrthancStone::CairoContext& context,
                             const ViewportGeometry& view);

    virtual void SetLayerStyle(const RenderStyle& style);

    virtual const OrthancStone::CoordinateSystem3D& GetLayerPlane()
    {
      return plane_;
    }
    
    virtual bool IsFullQuality()
    {
      return true;
    }
  };
}