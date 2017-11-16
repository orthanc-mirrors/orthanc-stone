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

        cairo_t* cr = context.GetObject();

        for (size_t k = 0; k < structureSet_.GetStructureCount(); k++)
        {
          /*if (structureSet_.GetStructureName(k) != "CORD")
          {
            continue;
            }*/
          
          std::vector< std::vector<DicomStructureSet::PolygonPoint> >  polygons;

          if (structureSet_.ProjectStructure(polygons, k, slice_))
          {
            uint8_t red, green, blue;
            structureSet_.GetStructureColor(red, green, blue, k);
            context.SetSourceColor(red, green, blue);

            for (size_t i = 0; i < polygons.size(); i++)
            {
              cairo_move_to(cr, polygons[i][0].first, polygons[i][0].second);

              for (size_t j = 1; j < polygons[i].size(); j++)
              {
                cairo_line_to(cr, polygons[i][j].first, polygons[i][j].second);
              }

              cairo_line_to(cr, polygons[i][0].first, polygons[i][0].second);
              cairo_stroke(cr);
            }
          }
        }
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
    if (loader_.HasStructureSet())
    {
      NotifyLayerReady(new Renderer(loader_.GetStructureSet(), viewportSlice), viewportSlice, false);
    }
  }
}
