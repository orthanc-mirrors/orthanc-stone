/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2021 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include <DicomFormat/DicomMap.h>


namespace OrthancStone
{
  /**
    Stores an indexed collection of DicomMap objects. The index is a 
    user-specified DicomTag.
  */
  class LoadedDicomResources : public boost::noncopyable
  {
  private:
    typedef std::map<std::string, Orthanc::DicomMap*>  Resources;

    Orthanc::DicomTag                indexedTag_;
    Resources                        resources_;
    std::vector<Orthanc::DicomMap*>  flattened_;

    void Flatten();

    void AddFromDicomWebInternal(const Json::Value& dicomweb);

  public:
    explicit LoadedDicomResources(const Orthanc::DicomTag& indexedTag) :
      indexedTag_(indexedTag)
    {
    }

    // Re-index another set of resources using another tag
    LoadedDicomResources(const LoadedDicomResources& other,
                         const Orthanc::DicomTag& indexedTag);

    ~LoadedDicomResources()
    {
      Clear();
    }

    const Orthanc::DicomTag& GetIndexedTag() const
    {
      return indexedTag_;
    }
  
    void Clear();

    size_t GetSize() const
    {
      return resources_.size();
    }

    Orthanc::DicomMap& GetResource(size_t index);

    bool HasResource(const std::string& id) const
    {
      return resources_.find(id) != resources_.end();
    }

    void MergeResource(Orthanc::DicomMap& target,
                       const std::string& id) const;
  
    bool LookupStringValue(std::string& target,
                           const std::string& id,
                           const Orthanc::DicomTag& tag) const;

    void AddResource(const Orthanc::DicomMap& dicom);

    void AddFromOrthanc(const Json::Value& tags);
  
    void AddFromDicomWeb(const Json::Value& dicomweb);

    bool LookupTagValueConsensus(std::string& target,
                                 const Orthanc::DicomTag& tag) const;
  };
}
