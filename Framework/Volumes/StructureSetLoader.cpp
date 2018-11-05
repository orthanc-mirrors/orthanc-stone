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
  
  StructureSetLoader::StructureSetLoader(MessageBroker& broker, OrthancApiClient& orthanc) :
    OrthancStone::IObserver(broker),
    orthanc_(orthanc)
  {
  }
  

  void StructureSetLoader::OnReferencedSliceLoaded(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    OrthancPlugins::FullOrthancDataset dataset(message.GetJson());

    Orthanc::DicomMap slice;
    MessagingToolbox::ConvertDataset(slice, dataset);
    structureSet_->AddReferencedSlice(slice);

    VolumeLoaderBase::NotifyContentChange();
  }

  void StructureSetLoader::OnStructureSetLoaded(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    OrthancPlugins::FullOrthancDataset dataset(message.GetJson());
    structureSet_.reset(new DicomStructureSet(dataset));

    std::set<std::string> instances;
    structureSet_->GetReferencedInstances(instances);

    for (std::set<std::string>::const_iterator it = instances.begin();
         it != instances.end(); ++it)
    {
      orthanc_.PostBinaryAsyncExpectJson("/tools/lookup", *it,
                            new Callable<StructureSetLoader, OrthancApiClient::JsonResponseReadyMessage>(*this, &StructureSetLoader::OnLookupCompleted));
    }

    VolumeLoaderBase::NotifyGeometryReady();
  }

  void StructureSetLoader::OnLookupCompleted(const OrthancApiClient::JsonResponseReadyMessage& message)
  {
    const Json::Value& lookup = message.GetJson();

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
    orthanc_.GetJsonAsync("/instances/" + instance + "/tags",
                          new Callable<StructureSetLoader, OrthancApiClient::JsonResponseReadyMessage>(*this, &StructureSetLoader::OnReferencedSliceLoaded));
  }

  void StructureSetLoader::ScheduleLoadInstance(const std::string& instance)
  {
    if (structureSet_.get() != NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      orthanc_.GetJsonAsync("/instances/" + instance + "/tags?ignore-length=3006-0050",
                            new Callable<StructureSetLoader, OrthancApiClient::JsonResponseReadyMessage>(*this, &StructureSetLoader::OnStructureSetLoaded));
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
