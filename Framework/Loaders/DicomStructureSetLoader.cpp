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


#include "DicomStructureSetLoader.h"

#include "../Scene2D/PolylineSceneLayer.h"
#include "../Toolbox/GeometryToolbox.h"

namespace OrthancStone
{
  class DicomStructureSetLoader::AddReferencedInstance : public LoaderStateMachine::State
  {
  private:
    std::string instanceId_;
      
  public:
    AddReferencedInstance(DicomStructureSetLoader& that,
                          const std::string& instanceId) :
      State(that),
      instanceId_(instanceId)
    {
    }

    virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      Json::Value tags;
      message.ParseJsonBody(tags);
        
      Orthanc::DicomMap dicom;
      dicom.FromDicomAsJson(tags);

      DicomStructureSetLoader& loader = GetLoader<DicomStructureSetLoader>();
      loader.content_->AddReferencedSlice(dicom);

      loader.countProcessedInstances_ ++;
      assert(loader.countProcessedInstances_ <= loader.countReferencedInstances_);

      if (loader.countProcessedInstances_ == loader.countReferencedInstances_)
      {
        // All the referenced instances have been loaded, finalize the RT-STRUCT
        loader.content_->CheckReferencedSlices();
        loader.revision_++;
      }
    }
  };


  // State that converts a "SOP Instance UID" to an Orthanc identifier
  class DicomStructureSetLoader::LookupInstance : public LoaderStateMachine::State
  {
  private:
    std::string  sopInstanceUid_;
      
  public:
    LookupInstance(DicomStructureSetLoader& that,
                   const std::string& sopInstanceUid) :
      State(that),
      sopInstanceUid_(sopInstanceUid)
    {
    }

    virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      DicomStructureSetLoader& loader = GetLoader<DicomStructureSetLoader>();

      Json::Value lookup;
      message.ParseJsonBody(lookup);

      if (lookup.type() != Json::arrayValue ||
          lookup.size() != 1 ||
          !lookup[0].isMember("Type") ||
          !lookup[0].isMember("Path") ||
          lookup[0]["Type"].type() != Json::stringValue ||
          lookup[0]["ID"].type() != Json::stringValue ||
          lookup[0]["Type"].asString() != "Instance")
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);          
      }

      const std::string instanceId = lookup[0]["ID"].asString();

      {
        std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
        command->SetHttpHeader("Accept-Encoding", "gzip");
        command->SetUri("/instances/" + instanceId + "/tags");
        command->SetPayload(new AddReferencedInstance(loader, instanceId));
        Schedule(command.release());
      }
    }
  };


  class DicomStructureSetLoader::LoadStructure : public LoaderStateMachine::State
  {
  public:
    LoadStructure(DicomStructureSetLoader& that) :
    State(that)
    {
    }

    virtual void Handle(const OrthancRestApiCommand::SuccessMessage& message)
    {
      DicomStructureSetLoader& loader = GetLoader<DicomStructureSetLoader>();
        
      {
        OrthancPlugins::FullOrthancDataset dicom(message.GetAnswer());
        loader.content_.reset(new DicomStructureSet(dicom));
      }

      std::set<std::string> instances;
      loader.content_->GetReferencedInstances(instances);

      loader.countReferencedInstances_ = static_cast<unsigned int>(instances.size());

      for (std::set<std::string>::const_iterator
             it = instances.begin(); it != instances.end(); ++it)
      {
        std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
        command->SetUri("/tools/lookup");
        command->SetMethod(Orthanc::HttpMethod_Post);
        command->SetBody(*it);
        command->SetPayload(new LookupInstance(loader, *it));
        Schedule(command.release());
      }
    }
  };
    

  class DicomStructureSetLoader::Slice : public IExtractedSlice
  {
  private:
    const DicomStructureSet&  content_;
    uint64_t                  revision_;
    bool                      isValid_;
      
  public:
    Slice(const DicomStructureSet& content,
          uint64_t revision,
          const CoordinateSystem3D& cuttingPlane) :
      content_(content),
      revision_(revision)
    {
      bool opposite;

      const Vector normal = content.GetNormal();
      isValid_ = (
        GeometryToolbox::IsParallelOrOpposite(opposite, normal, cuttingPlane.GetNormal()) ||
        GeometryToolbox::IsParallelOrOpposite(opposite, normal, cuttingPlane.GetAxisX()) ||
        GeometryToolbox::IsParallelOrOpposite(opposite, normal, cuttingPlane.GetAxisY()));
    }
      
    virtual bool IsValid()
    {
      return isValid_;
    }

    virtual uint64_t GetRevision()
    {
      return revision_;
    }

    virtual ISceneLayer* CreateSceneLayer(const ILayerStyleConfigurator* configurator,
                                          const CoordinateSystem3D& cuttingPlane)
    {
      assert(isValid_);

      std::auto_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer);
      layer->SetThickness(2);

      for (size_t i = 0; i < content_.GetStructuresCount(); i++)
      {
        const Color& color = content_.GetStructureColor(i);

        std::vector< std::vector<DicomStructureSet::PolygonPoint> > polygons;
          
        if (content_.ProjectStructure(polygons, i, cuttingPlane))
        {
          for (size_t j = 0; j < polygons.size(); j++)
          {
            PolylineSceneLayer::Chain chain;
            chain.resize(polygons[j].size());
            
            for (size_t k = 0; k < polygons[j].size(); k++)
            {
              chain[k] = ScenePoint2D(polygons[j][k].first, polygons[j][k].second);
            }

            layer->AddChain(chain, true /* closed */, color);
          }
        }
      }

      return layer.release();
    }
  };
    

  DicomStructureSetLoader::DicomStructureSetLoader(IOracle& oracle,
                                                   IObservable& oracleObservable) :
    LoaderStateMachine(oracle, oracleObservable),
    revision_(0),
    countProcessedInstances_(0),
    countReferencedInstances_(0)
  {
  }
    
    
  void DicomStructureSetLoader::LoadInstance(const std::string& instanceId)
  {
    Start();
      
    instanceId_ = instanceId;
      
    {
      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetHttpHeader("Accept-Encoding", "gzip");
      command->SetUri("/instances/" + instanceId + "/tags?ignore-length=3006-0050");
      command->SetPayload(new LoadStructure(*this));
      Schedule(command.release());
    }
  }


  IVolumeSlicer::IExtractedSlice* DicomStructureSetLoader::ExtractSlice(const CoordinateSystem3D& cuttingPlane)
  {
    if (content_.get() == NULL)
    {
      // Geometry is not available yet
      return new IVolumeSlicer::InvalidSlice;
    }
    else
    {
      return new Slice(*content_, revision_, cuttingPlane);
    }
  }
}