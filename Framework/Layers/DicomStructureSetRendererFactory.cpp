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

#include "../../Resources/Orthanc/Core/OrthancException.h"

namespace OrthancStone
{
  class DicomStructureSetRendererFactory::Renderer : public ILayerRenderer
  {
  private:
    const DicomStructureSet&  structureSet_;
    SliceGeometry             slice_;
    bool                      visible_;

  public:
    Renderer(const DicomStructureSet& structureSet,
             const SliceGeometry& slice) :
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

    virtual void SetLayerStyle(const RenderStyle& style)
    {
      visible_ = style.visible_;
    }

    virtual bool IsFullQuality()
    {
      return true;
    }
  };


  ILayerRenderer* DicomStructureSetRendererFactory::CreateLayerRenderer(const SliceGeometry& displaySlice)
  {
    bool isOpposite;
    if (GeometryToolbox::IsParallelOrOpposite(isOpposite, displaySlice.GetNormal(), structureSet_.GetNormal()))
    {
      return new Renderer(structureSet_, displaySlice);
    }
    else
    {
      return NULL;
    }
  }


  ISliceableVolume& DicomStructureSetRendererFactory::GetSourceVolume() const
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
}
