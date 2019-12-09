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


#include "LoadedDicomResources.h"

#include <Core/OrthancException.h>

#include <cassert>


namespace OrthancStone
{
  void LoadedDicomResources::Flatten()
  {
    // Lazy generation of a "std::vector" from the "std::map"
    if (flattened_.empty())
    {
      flattened_.resize(resources_.size());

      size_t pos = 0;
      for (Resources::const_iterator it = resources_.begin(); it != resources_.end(); ++it)
      {
        assert(it->second != NULL);
        flattened_[pos++] = it->second;
      }
    }
    else
    {
      // No need to flatten
      assert(flattened_.size() == resources_.size());
    }
  }


  void LoadedDicomResources::AddFromDicomWebInternal(const Json::Value& dicomweb)
  {
    assert(dicomweb.type() == Json::objectValue);
    Orthanc::DicomMap dicom;
    dicom.FromDicomWeb(dicomweb);
    AddResource(dicom);
  }

  
  LoadedDicomResources::LoadedDicomResources(const LoadedDicomResources& other,
                                             const Orthanc::DicomTag& indexedTag) :
    indexedTag_(indexedTag)
  {
    for (Resources::const_iterator it = other.resources_.begin();
         it != other.resources_.end(); ++it)
    {
      assert(it->second != NULL);
      AddResource(*it->second);
    }
  }

  void LoadedDicomResources::Clear()
  {
    for (Resources::iterator it = resources_.begin(); it != resources_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }

    resources_.clear();
    flattened_.clear();
  }


  Orthanc::DicomMap& LoadedDicomResources::GetResource(size_t index)
  {
    Flatten();

    if (index >= flattened_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(flattened_[index] != NULL);
      return *flattened_[index];
    }
  }


  bool LoadedDicomResources::LookupStringValue(std::string& target,
                                               const std::string& id,
                                               const Orthanc::DicomTag& tag) const
  {
    Resources::const_iterator found = resources_.find(id);

    if (found == resources_.end())
    {
      return false;
    }
    else
    {
      assert(found->second != NULL);
      return found->second->LookupStringValue(target, tag, false);
    }  
  }

  
  void LoadedDicomResources::AddResource(const Orthanc::DicomMap& dicom)
  {
    std::string id;
    
    if (dicom.LookupStringValue(id, indexedTag_, false /* no binary value */) &&
        resources_.find(id) == resources_.end() /* Don't index twice the same resource */)
    {
      resources_[id] = dicom.Clone();
      flattened_.clear();   // Invalidate the flattened version 
    }
  }


  void LoadedDicomResources::AddFromOrthanc(const Json::Value& tags)
  {
    Orthanc::DicomMap dicom;
    dicom.FromDicomAsJson(tags);
    AddResource(dicom);
  }


  void LoadedDicomResources::AddFromDicomWeb(const Json::Value& dicomweb)
  {
    if (dicomweb.type() == Json::objectValue)
    {
      AddFromDicomWebInternal(dicomweb);
    }
    else if (dicomweb.type() == Json::arrayValue)
    {
      for (Json::Value::ArrayIndex i = 0; i < dicomweb.size(); i++)
      {
        if (dicomweb[i].type() == Json::objectValue)
        {
          AddFromDicomWebInternal(dicomweb[i]);
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }
      }
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
    }
  }


  bool LoadedDicomResources::LookupTagValueConsensus(std::string& target,
                                                     const Orthanc::DicomTag& tag) const
  {
    typedef std::map<std::string, unsigned int>  Counter;

    Counter counter;

    for (Resources::const_iterator it = resources_.begin(); it != resources_.end(); ++it)
    {
      assert(it->second != NULL);
      
      std::string value;
      if (it->second->LookupStringValue(value, tag, false))
      {
        Counter::iterator found = counter.find(value);
        if (found == counter.end())
        {
          counter[value] = 1;
        }
        else
        {
          found->second ++;
        }
      }
    }

    Counter::const_iterator best = counter.end();
    
    for (Counter::const_iterator it = counter.begin(); it != counter.end(); ++it)
    {
      if (best == counter.end() ||
          best->second < it->second)
      {
        best = it;
      }
    }

    if (best == counter.end())
    {
      return false;
    }
    else
    {
      target = best->first;
      return true;
    }
  }
}
