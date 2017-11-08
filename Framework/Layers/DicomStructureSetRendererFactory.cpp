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

#include "../Toolbox/MessagingToolbox.h"

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


  class DicomStructureSetRendererFactory::Operation : public Orthanc::IDynamicObject
  {
  public:
    enum Type
    {
      Type_LoadStructureSet,
      Type_LookupSopInstanceUid,
      Type_LoadReferencedSlice
    };
    
  private:
    Type         type_;
    std::string  value_;

  public:
    Operation(Type type,
              const std::string& value) :
      type_(type),
      value_(value)
    {
    }

    Type GetType() const
    {
      return type_;
    }

    const std::string& GetIdentifier() const
    {
      return value_;
    }
  };


  void DicomStructureSetRendererFactory::NotifyError(const std::string& uri,
                                                     Orthanc::IDynamicObject* payload)
  {
    // TODO
  }

  
  void DicomStructureSetRendererFactory::NotifySuccess(const std::string& uri,
                                                       const void* answer,
                                                       size_t answerSize,
                                                       Orthanc::IDynamicObject* payload)
  {
    std::auto_ptr<Operation> op(dynamic_cast<Operation*>(payload));

    switch (op->GetType())
    {
      case Operation::Type_LoadStructureSet:
      {
        OrthancPlugins::FullOrthancDataset dataset(answer, answerSize);
        structureSet_.reset(new DicomStructureSet(dataset));

        std::set<std::string> instances;
        structureSet_->GetReferencedInstances(instances);

        for (std::set<std::string>::const_iterator it = instances.begin();
             it != instances.end(); ++it)
        {
          orthanc_.SchedulePostRequest(*this, "/tools/lookup", *it,
                                       new Operation(Operation::Type_LookupSopInstanceUid, *it));
        }
        
        break;
      }
        
      case Operation::Type_LookupSopInstanceUid:
      {
        Json::Value lookup;
        
        if (MessagingToolbox::ParseJson(lookup, answer, answerSize))
        {
          if (lookup.type() != Json::arrayValue ||
              lookup.size() != 1 ||
              !lookup[0].isMember("Type") ||
              !lookup[0].isMember("Path") ||
              lookup[0]["Type"].type() != Json::stringValue ||
              lookup[0]["ID"].type() != Json::stringValue ||
              lookup[0]["Type"].asString() != "Instance")
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);          
          }

          const std::string& instance = lookup[0]["ID"].asString();
          orthanc_.ScheduleGetRequest(*this, "/instances/" + instance + "/tags",
                                      new Operation(Operation::Type_LoadReferencedSlice, instance));
        }
        else
        {
          // TODO
        }
        
        break;
      }

      case Operation::Type_LoadReferencedSlice:
      {
        OrthancPlugins::FullOrthancDataset dataset(answer, answerSize);

        Orthanc::DicomMap slice;
        MessagingToolbox::ConvertDataset(slice, dataset);
        structureSet_->AddReferencedSlice(slice);

        LayerSourceBase::NotifyContentChange();

        break;
      }
      
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  } 

  
  DicomStructureSetRendererFactory::DicomStructureSetRendererFactory(IWebService& orthanc,
                                                                     const std::string& instance) :
    orthanc_(orthanc)
  {
    const std::string uri = "/instances/" + instance + "/tags?ignore-length=3006-0050";
    orthanc_.ScheduleGetRequest(*this, uri, new Operation(Operation::Type_LoadStructureSet, instance));
  }
  
  
  void DicomStructureSetRendererFactory::ScheduleLayerCreation(const CoordinateSystem3D& viewportSlice)
  {
    if (structureSet_.get() != NULL)
    {
      NotifyLayerReady(new Renderer(*structureSet_, viewportSlice), viewportSlice, false);
    }
  }
}
