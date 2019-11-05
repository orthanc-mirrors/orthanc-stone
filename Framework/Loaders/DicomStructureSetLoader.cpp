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
#include "../StoneException.h"
#include "../Toolbox/GeometryToolbox.h"

#include <Core/Toolbox.h>

#include <algorithm>

#if 0
bool logbgo233 = false;
bool logbgo115 = false;
#endif

namespace OrthancStone
{

#if 0
  void DumpDicomMap(std::ostream& o, const Orthanc::DicomMap& dicomMap)
  {
    using namespace std;
    //ios_base::fmtflags state = o.flags();
    //o.flags(ios::right | ios::hex);
    //o << "(" << setfill('0') << setw(4) << tag.GetGroup()
    //  << "," << setw(4) << tag.GetElement() << ")";
    //o.flags(state);
    Json::Value val;
    dicomMap.Serialize(val);
    o << val;
    //return o;
  }
#endif


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
        loader.SetStructuresReady();
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
#if 0
      LOG(TRACE) << "DicomStructureSetLoader::LookupInstance::Handle() (SUCCESS)";
#endif
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
        std::stringstream msg;
        msg << "Unknown resource! message.GetAnswer() = " << message.GetAnswer() << " message.GetAnswerHeaders() = ";
        for (OrthancRestApiCommand::HttpHeaders::const_iterator it = message.GetAnswerHeaders().begin();
             it != message.GetAnswerHeaders().end(); ++it)
        {
          msg << "\nkey: \"" << it->first << "\" value: \"" << it->second << "\"\n";
        }
        const std::string msgStr = msg.str();
        LOG(ERROR) << msgStr;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);          
      }

      const std::string instanceId = lookup[0]["ID"].asString();

      {
        std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
        command->SetHttpHeader("Accept-Encoding", "gzip");
        std::string uri = "/instances/" + instanceId + "/tags";
        command->SetUri(uri);
        command->AcquirePayload(new AddReferencedInstance(loader, instanceId));
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
#if 0
      if (logbgo115)
        LOG(TRACE) << "DicomStructureSetLoader::LoadStructure::Handle() (SUCCESS)";
#endif
      DicomStructureSetLoader& loader = GetLoader<DicomStructureSetLoader>();
        
      {
        OrthancPlugins::FullOrthancDataset dicom(message.GetAnswer());
        loader.content_.reset(new DicomStructureSet(dicom));
        size_t structureCount = loader.content_->GetStructuresCount();
        loader.structureVisibility_.resize(structureCount);
        bool everythingVisible = false;
        if ((loader.initiallyVisibleStructures_.size() == 1)
          && (loader.initiallyVisibleStructures_[0].size() == 1)
          && (loader.initiallyVisibleStructures_[0][0] == '*'))
        {
          everythingVisible = true;
        }

        for (size_t i = 0; i < structureCount; ++i)
        {
          // if a single "*" string is supplied, this means we want everything 
          // to be visible...
          if(everythingVisible)
          {
            loader.structureVisibility_.at(i) = true;
          }
          else
          {
            // otherwise, we only enable visibility for those structures whose 
            // names are mentioned in the initiallyVisibleStructures_ array
            const std::string& structureName = loader.content_->GetStructureName(i);

            std::vector<std::string>::iterator foundIt =
              std::find(
                loader.initiallyVisibleStructures_.begin(),
                loader.initiallyVisibleStructures_.end(),
                structureName);
            std::vector<std::string>::iterator endIt = loader.initiallyVisibleStructures_.end();
            if (foundIt != endIt)
              loader.structureVisibility_.at(i) = true;
            else
              loader.structureVisibility_.at(i) = false;
          }
        }
      }

      // Some (admittedly invalid) Dicom files have empty values in the 
      // 0008,1155 tag. We try our best to cope with this.
      std::set<std::string> instances;
      std::set<std::string> nonEmptyInstances;
      loader.content_->GetReferencedInstances(instances);
      for (std::set<std::string>::const_iterator
        it = instances.begin(); it != instances.end(); ++it)
      {
        std::string instance = Orthanc::Toolbox::StripSpaces(*it);
        if(instance != "")
          nonEmptyInstances.insert(instance);
      }

      loader.countReferencedInstances_ = 
        static_cast<unsigned int>(nonEmptyInstances.size());

      for (std::set<std::string>::const_iterator
        it = nonEmptyInstances.begin(); it != nonEmptyInstances.end(); ++it)
      {
        std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
        command->SetUri("/tools/lookup");
        command->SetMethod(Orthanc::HttpMethod_Post);
        command->SetBody(*it);
        command->AcquirePayload(new LookupInstance(loader, *it));
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
    std::vector<bool>         visibility_;
      
  public:
    /**
    The visibility vector must either:
    - be empty
    or
    - contain the same number of items as the number of structures in the 
      structure set.
    In the first case (empty vector), all the structures are displayed.
    In the second case, the visibility of each structure is defined by the 
    content of the vector at the corresponding index.
    */
    Slice(const DicomStructureSet& content,
          uint64_t revision,
          const CoordinateSystem3D& cuttingPlane,
          std::vector<bool> visibility = std::vector<bool>()) 
      : content_(content)
      , revision_(revision)
      , visibility_(visibility)
    {
      ORTHANC_ASSERT((visibility_.size() == content_.GetStructuresCount())
        || (visibility_.size() == 0u));

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
        if ((visibility_.size() == 0) || visibility_.at(i))
        {
          const Color& color = content_.GetStructureColor(i);

#ifdef USE_BOOST_UNION_FOR_POLYGONS 
          std::vector< std::vector<Point2D> > polygons;

          if (content_.ProjectStructure(polygons, i, cuttingPlane))
          {
            for (size_t j = 0; j < polygons.size(); j++)
            {
              PolylineSceneLayer::Chain chain;
              chain.resize(polygons[j].size());

              for (size_t k = 0; k < polygons[j].size(); k++)
              {
                chain[k] = ScenePoint2D(polygons[j][k].x, polygons[j][k].y);
    }

              layer->AddChain(chain, true /* closed */, color);
  }
        }
#else
          std::vector< std::pair<Point2D, Point2D> > segments;

          if (content_.ProjectStructure(segments, i, cuttingPlane))
          {
            for (size_t j = 0; j < segments.size(); j++)
            {
              PolylineSceneLayer::Chain chain;
              chain.resize(2);

              chain[0] = ScenePoint2D(segments[j].first.x, segments[j].first.y);
              chain[1] = ScenePoint2D(segments[j].second.x, segments[j].second.y);

              layer->AddChain(chain, false /* NOT closed */, color);
            }
          }
#endif        
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
    countReferencedInstances_(0),
    structuresReady_(false)
  {
  }
    
    
  void DicomStructureSetLoader::SetStructureDisplayState(size_t structureIndex, bool display)
  {
    structureVisibility_.at(structureIndex) = display;
    revision_++;
  }

  DicomStructureSetLoader::~DicomStructureSetLoader()
  {
    LOG(TRACE) << "DicomStructureSetLoader::~DicomStructureSetLoader()";
  }

  void DicomStructureSetLoader::LoadInstance(
    const std::string& instanceId, 
    const std::vector<std::string>& initiallyVisibleStructures)
  {
    Start();
      
    instanceId_ = instanceId;
    initiallyVisibleStructures_ = initiallyVisibleStructures;

    {
      std::auto_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
      command->SetHttpHeader("Accept-Encoding", "gzip");

      std::string uri = "/instances/" + instanceId + "/tags?ignore-length=3006-0050";

      command->SetUri(uri);
      command->AcquirePayload(new LoadStructure(*this));
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
      return new Slice(*content_, revision_, cuttingPlane, structureVisibility_);
    }
  }

  void DicomStructureSetLoader::SetStructuresReady()
  {
    ORTHANC_ASSERT(!structuresReady_);
    structuresReady_ = true;
    BroadcastMessage(DicomStructureSetLoader::StructuresReady(*this));
  }

  bool DicomStructureSetLoader::AreStructuresReady() const
  {
    return structuresReady_;
  }

}
