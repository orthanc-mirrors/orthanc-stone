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

#include "DicomStructureSetSlicer.h"

#include "../../Toolbox/DicomStructureSet.h"

namespace Deprecated
{
  class DicomStructureSetSlicer::Renderer : public ILayerRenderer
  {
  private:
    class Structure
    {
    private:
      bool         visible_;
      uint8_t      red_;
      uint8_t      green_;
      uint8_t      blue_;
      std::string  name_;

#if USE_BOOST_UNION_FOR_POLYGONS == 1
      std::vector< std::vector<OrthancStone::Point2D> > polygons_;
#else
      std::vector< std::pair<OrthancStone::Point2D, OrthancStone::Point2D> > segments_;
#endif
      
    public:
      Structure(OrthancStone::DicomStructureSet& structureSet,
                const OrthancStone::CoordinateSystem3D& plane,
                size_t index) :
        name_(structureSet.GetStructureName(index))
      {
        structureSet.GetStructureColor(red_, green_, blue_, index);

#if USE_BOOST_UNION_FOR_POLYGONS == 1
        visible_ = structureSet.ProjectStructure(polygons_, index, plane);
#else
        visible_ = structureSet.ProjectStructure(segments_, index, plane);
#endif
      }

      void Render(OrthancStone::CairoContext& context)
      {
        if (visible_)
        {
          cairo_t* cr = context.GetObject();
        
          context.SetSourceColor(red_, green_, blue_);

#if USE_BOOST_UNION_FOR_POLYGONS == 1
          for (size_t i = 0; i < polygons_.size(); i++)
          {
            cairo_move_to(cr, polygons_[i][0].x, polygons_[i][0].y);
            for (size_t j = 0; j < polygons_[i].size(); j++)
            {
              cairo_line_to(cr, polygons_[i][j].x, polygons_[i][j].y);
            }
            cairo_line_to(cr, polygons_[i][0].x, polygons_[i][0].y);
            cairo_stroke(cr);
          }
#else
          for (size_t i = 0; i < segments_.size(); i++)
          {
            cairo_move_to(cr, segments_[i].first.x, segments_[i].first.y);
            cairo_line_to(cr, segments_[i].second.x, segments_[i].second.y);
            cairo_stroke(cr);
          }
#endif
        }
      }
    };

    typedef std::list<Structure*>  Structures;
    
    OrthancStone::CoordinateSystem3D  plane_;
    Structures          structures_;
    
  public:
    Renderer(OrthancStone::DicomStructureSet& structureSet,
             const OrthancStone::CoordinateSystem3D& plane) :
      plane_(plane)
    {
      for (size_t k = 0; k < structureSet.GetStructuresCount(); k++)
      {
        structures_.push_back(new Structure(structureSet, plane, k));
      }
    }

    virtual ~Renderer()
    {
      for (Structures::iterator it = structures_.begin();
           it != structures_.end(); ++it)
      {
        delete *it;
      }
    }

    virtual bool RenderLayer(OrthancStone::CairoContext& context,
                             const ViewportGeometry& view)
    {
      cairo_set_line_width(context.GetObject(), 2.0f / view.GetZoom());

      for (Structures::const_iterator it = structures_.begin();
           it != structures_.end(); ++it)
      {
        assert(*it != NULL);
        (*it)->Render(context);
      }

      return true;
    }

    virtual const OrthancStone::CoordinateSystem3D& GetLayerPlane()
    {
      return plane_;
    }

    virtual void SetLayerStyle(const RenderStyle& style)
    {
    }
    
    virtual bool IsFullQuality()
    {
      return true;
    }
  };


  class DicomStructureSetSlicer::RendererFactory : public LayerReadyMessage::IRendererFactory
  {
  private:
    OrthancStone::DicomStructureSet&         structureSet_;
    const OrthancStone::CoordinateSystem3D&  plane_;

  public:
    RendererFactory(OrthancStone::DicomStructureSet& structureSet,
                    const OrthancStone::CoordinateSystem3D&  plane) :
      structureSet_(structureSet),
      plane_(plane)
    {
    }

    virtual ILayerRenderer* CreateRenderer() const
    {
      return new Renderer(structureSet_, plane_);
    }
  };
  

  DicomStructureSetSlicer::DicomStructureSetSlicer(StructureSetLoader& loader) :
    loader_(loader)
  {
    Register<StructureSetLoader::ContentChangedMessage>(loader_, &DicomStructureSetSlicer::OnStructureSetLoaded);
  }


  void DicomStructureSetSlicer::ScheduleLayerCreation(const OrthancStone::CoordinateSystem3D& viewportPlane)
  {
    if (loader_.HasStructureSet())
    {
      RendererFactory factory(loader_.GetStructureSet(), viewportPlane);
      BroadcastMessage(IVolumeSlicer::LayerReadyMessage(*this, factory, viewportPlane));
    }
  }
}
