/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

namespace OrthancStone
{
  class DicomStructureSetSlicer::Renderer : public ILayerRenderer
  {
  private:
    class Structure
    {
    private:
      bool                                                         visible_;
      uint8_t                                                      red_;
      uint8_t                                                      green_;
      uint8_t                                                      blue_;
      std::string                                                  name_;
      std::vector< std::vector<DicomStructureSet::PolygonPoint> >  polygons_;

    public:
      Structure(DicomStructureSet& structureSet,
                const CoordinateSystem3D& plane,
                size_t index) :
        name_(structureSet.GetStructureName(index))
      {
        structureSet.GetStructureColor(red_, green_, blue_, index);
        visible_ = structureSet.ProjectStructure(polygons_, index, plane);
      }

      void Render(CairoContext& context)
      {
        if (visible_)
        {
          cairo_t* cr = context.GetObject();
        
          context.SetSourceColor(red_, green_, blue_);

          for (size_t i = 0; i < polygons_.size(); i++)
          {
            cairo_move_to(cr, polygons_[i][0].first, polygons_[i][0].second);

            for (size_t j = 1; j < polygons_[i].size(); j++)
            {
              cairo_line_to(cr, polygons_[i][j].first, polygons_[i][j].second);
            }

            cairo_line_to(cr, polygons_[i][0].first, polygons_[i][0].second);
            cairo_stroke(cr);
          }
        }
      }
    };

    typedef std::list<Structure*>  Structures;
    
    CoordinateSystem3D  plane_;
    Structures          structures_;
    
  public:
    Renderer(DicomStructureSet& structureSet,
             const CoordinateSystem3D& plane) :
      plane_(plane)
    {
      for (size_t k = 0; k < structureSet.GetStructureCount(); k++)
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

    virtual bool RenderLayer(CairoContext& context,
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

    virtual const CoordinateSystem3D& GetLayerPlane()
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
    DicomStructureSet&         structureSet_;
    const CoordinateSystem3D&  plane_;

  public:
    RendererFactory(DicomStructureSet& structureSet,
                    const CoordinateSystem3D&  plane) :
      structureSet_(structureSet),
      plane_(plane)
    {
    }

    virtual ILayerRenderer* CreateRenderer() const
    {
      return new Renderer(structureSet_, plane_);
    }
  };
  

  DicomStructureSetSlicer::DicomStructureSetSlicer(MessageBroker& broker,
                                                   StructureSetLoader& loader) :
    IVolumeSlicer(broker),
    IObserver(broker),
    loader_(loader)
  {
    loader_.RegisterObserverCallback(
      new Callable<DicomStructureSetSlicer, StructureSetLoader::ContentChangedMessage>
      (*this, &DicomStructureSetSlicer::OnStructureSetLoaded));
  }


  void DicomStructureSetSlicer::ScheduleLayerCreation(const CoordinateSystem3D& viewportPlane)
  {
    if (loader_.HasStructureSet())
    {
      RendererFactory factory(loader_.GetStructureSet(), viewportPlane);
      EmitMessage(IVolumeSlicer::LayerReadyMessage(*this, factory, viewportPlane));
    }
  }
}