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

#include "ILayerRenderer.h"
#include "../Toolbox/Slice.h"

namespace Deprecated
{
  class SliceOutlineRenderer : public ILayerRenderer
  {
  private:
    OrthancStone::CoordinateSystem3D  geometry_;
    double              pixelSpacingX_;
    double              pixelSpacingY_;
    unsigned int        width_;
    unsigned int        height_;
    RenderStyle         style_;

  public:
    SliceOutlineRenderer(const Slice& slice) :
      geometry_(slice.GetGeometry()),
      pixelSpacingX_(slice.GetPixelSpacingX()),
      pixelSpacingY_(slice.GetPixelSpacingY()),
      width_(slice.GetWidth()),
      height_(slice.GetHeight())
    {
    }

    virtual bool RenderLayer(OrthancStone::CairoContext& context,
                             const ViewportGeometry& view);

    virtual void SetLayerStyle(const RenderStyle& style)
    {
      style_ = style;
    }

    virtual const OrthancStone::CoordinateSystem3D& GetLayerSlice()
    {
      return geometry_;
    }

    virtual bool IsFullQuality()
    {
      return true;
    }
  };
}