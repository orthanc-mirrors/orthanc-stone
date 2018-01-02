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


#include "StructureSetLoader.h"

#include "../Toolbox/MessagingToolbox.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  class StructureSetLoader::Operation : public Orthanc::IDynamicObject
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


  void StructureSetLoader::NotifyError(const std::string& uri,
                                       Orthanc::IDynamicObject* payload)
  {
    // TODO
  }

  
  void StructureSetLoader::NotifySuccess(const std::string& uri,
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
        
        VolumeLoaderBase::NotifyGeometryReady();

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

        VolumeLoaderBase::NotifyContentChange();

        break;
      }
      
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  } 

  
  StructureSetLoader::StructureSetLoader(IWebService& orthanc) :
    orthanc_(orthanc)
  {
  }
  

  void StructureSetLoader::ScheduleLoadInstance(const std::string& instance)
  {
    if (structureSet_.get() != NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      const std::string uri = "/instances/" + instance + "/tags?ignore-length=3006-0050";
      orthanc_.ScheduleGetRequest(*this, uri, new Operation(Operation::Type_LoadStructureSet, instance));
    }
  }


  DicomStructureSet& StructureSetLoader::GetStructureSet()
  {
    if (structureSet_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return *structureSet_;
    }
  }
}
