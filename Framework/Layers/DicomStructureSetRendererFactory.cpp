/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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


#include "DicomStructureSetRendererFactory.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  class DicomStructureSetRendererFactory::Renderer : public ILayerRenderer
  {
  private:
    DicomStructureSet&  structureSet_;
    CoordinateSystem3D  slice_;
    bool                visible_;

  public:
    Renderer(DicomStructureSet& structureSet,
             const CoordinateSystem3D& slice) :
      structureSet_(structureSet),
      slice_(slice),
      visible_(true)
    {
    }

    virtual bool RenderLayer(CairoContext& context,
                             const ViewportGeometry& view)
    {
      if (visible_)
      {
        cairo_set_line_width(context.GetObject(), 3.0f / view.GetZoom());
        structureSet_.Render(context, slice_);
      }

      return true;
    }

    virtual const CoordinateSystem3D& GetLayerSlice()
    {
      return slice_;
    }

    virtual void SetLayerStyle(const RenderStyle& style)
    {
      visible_ = style.visible_;
    }

    virtual bool IsFullQuality()
    {
      return true;
    }
  };


  void DicomStructureSetRendererFactory::ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice)
  {
    bool isOpposite;
    if (GeometryToolbox::IsParallelOrOpposite(isOpposite, viewportSlice.GetNormal(), structureSet_.GetNormal()))
    {
      NotifyLayerReady(new Renderer(structureSet_, viewportSlice), viewportSlice, false);
    }
  }
}
