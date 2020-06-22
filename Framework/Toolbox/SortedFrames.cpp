/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "SortedFrames.h"

#include <OrthancException.h>

#include "GeometryToolbox.h"

namespace OrthancStone
{
  SortedFrames::Instance::Instance(const Orthanc::DicomMap& tags)
  {
    tags_.Assign(tags);

    if (!tags.LookupStringValue(sopInstanceUid_, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    uint32_t tmp;
    if (tags.ParseUnsignedInteger32(tmp, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES))
    {
      numberOfFrames_ = tmp;
    }
    else
    {
      numberOfFrames_ = 1;
    }

    hasPosition_ = (
      LinearAlgebra::ParseVector(position_, tags, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT) &&
      position_.size() == 3 &&
      GeometryToolbox::ComputeNormal(normal_, tags));
  }


  const Vector& SortedFrames::Instance::GetNormal() const
  {
    if (hasPosition_)
    {
      return normal_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  const Vector& SortedFrames::Instance::GetPosition() const
  {
    if (hasPosition_)
    {
      return position_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  
  SortedFrames::Frame::Frame(const Instance& instance,
                             unsigned int frameIndex) :
    instance_(&instance),
    frameIndex_(frameIndex)
  {
    if (frameIndex >= instance.GetNumberOfFrames())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  const SortedFrames::Instance& SortedFrames::GetInstance(size_t index) const
  {
    if (index >= instances_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(instances_[index] != NULL);
      return *instances_[index];
    }
  }

  
  const SortedFrames::Frame& SortedFrames::GetFrame(size_t index) const
  {
    if (!sorted_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                      "Sort() has not been called");
    }
    if (index >= frames_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return frames_[index];
    }    
  }

  
  void SortedFrames::Clear()
  {
    for (size_t i = 0; i < instances_.size(); i++)
    {
      assert(instances_[i] != NULL);
      delete instances_[i];
    }

    studyInstanceUid_.clear();
    seriesInstanceUid_.clear();
    frames_.clear();
    sorted_ = true;
  }


  void SortedFrames::AddInstance(const Orthanc::DicomMap& tags)
  {
    std::unique_ptr<Instance> instance(new Instance(tags));

    std::string studyInstanceUid, seriesInstanceUid;
    if (!tags.LookupStringValue(studyInstanceUid, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) ||
        !tags.LookupStringValue(seriesInstanceUid, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
    
    if (instances_.empty())
    {
      studyInstanceUid_ = studyInstanceUid;
      seriesInstanceUid_ = seriesInstanceUid;
    }
    else
    {
      if (studyInstanceUid_ != studyInstanceUid ||
          seriesInstanceUid_ != seriesInstanceUid)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                        "Mixing instances from different series");
      }
    }

    instances_.push_back(instance.release());
    sorted_ = false;
    frames_.clear();
  }


  void SortedFrames::AddFramesOfInstance(std::set<size_t>& remainingInstances,
                                         size_t index)
  {
    assert(instances_[index] != NULL);
    const Instance& instance = *instances_[index];
    
    for (unsigned int i = 0; i < instance.GetNumberOfFrames(); i++)
    {
      frames_.push_back(Frame(instance, i));
    }

    assert(remainingInstances.find(index) != remainingInstances.end());
    remainingInstances.erase(index);
  }


  namespace
  {
    template<typename T>
    class SortableItem
    {
    private:
      T            value_;
      size_t       instance_;
      std::string  sopInstanceUid_;

    public:
      SortableItem(const T& value,
                   size_t instance,
                   const std::string& sopInstanceUid) :
        value_(value),
        instance_(instance),
        sopInstanceUid_(sopInstanceUid)
      {
      }

      size_t GetInstanceIndex() const
      {
        return instance_;
      }

      bool operator< (const SortableItem& other) const
      {
        return (value_ < other.value_ ||
                (value_ == other.value_ &&
                 sopInstanceUid_ < other.sopInstanceUid_));
      }
    };
  }


  void SortedFrames::SortUsingIntegerTag(std::set<size_t>& remainingInstances,
                                         const Orthanc::DicomTag& tag)
  {
    std::vector< SortableItem<int32_t> > items;
    items.reserve(remainingInstances.size());

    for (std::set<size_t>::const_iterator it = remainingInstances.begin();
         it != remainingInstances.end(); ++it)
    {
      assert(instances_[*it] != NULL);
      const Instance& instance = *instances_[*it];

      int32_t value;
      std::string sopInstanceUid;
      if (instance.GetTags().ParseInteger32(value, tag) &&
          instance.GetTags().LookupStringValue(
            sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
      {
        items.push_back(SortableItem<int32_t>(value, *it, sopInstanceUid));
      }
    }
    
    std::sort(items.begin(), items.end());

    for (size_t i = 0; i < items.size(); i++)
    {
      AddFramesOfInstance(remainingInstances, items[i].GetInstanceIndex());
    }
  }


  void SortedFrames::SortUsingSopInstanceUid(std::set<size_t>& remainingInstances)
  {
    std::vector<SortableItem<int32_t> > items;
    items.reserve(remainingInstances.size());

    for (std::set<size_t>::const_iterator it = remainingInstances.begin();
         it != remainingInstances.end(); ++it)
    {
      assert(instances_[*it] != NULL);
      const Instance& instance = *instances_[*it];

      std::string sopInstanceUid;
      if (instance.GetTags().LookupStringValue(
            sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
      {
        items.push_back(SortableItem<int32_t>(0 /* arbitrary value */, *it, sopInstanceUid));
      }
    }
    
    std::sort(items.begin(), items.end());

    for (size_t i = 0; i < items.size(); i++)
    {
      AddFramesOfInstance(remainingInstances, items[i].GetInstanceIndex());
    }
  }


  void SortedFrames::SortUsing3DLocation(std::set<size_t>& remainingInstances)
  {
    /**
     * Compute the mean of the normal vectors, using the recursive
     * formula for arithmetic means for numerical stability.
     * https://diego.assencio.com/?index=c34d06f4f4de2375658ed41f70177d59
     **/
      
    Vector meanNormal;
    LinearAlgebra::AssignVector(meanNormal, 0, 0, 0);

    unsigned int n = 0;

    for (std::set<size_t>::const_iterator it = remainingInstances.begin();
         it != remainingInstances.end(); ++it)
    {
      assert(instances_[*it] != NULL);
      const Instance& instance = *instances_[*it];

      if (instance.HasPosition())
      {
        n += 1;
        meanNormal += (instance.GetNormal() - meanNormal) / static_cast<float>(n);
      }
    }

    std::vector<SortableItem<float> > items;
    items.reserve(n);
      
    for (std::set<size_t>::const_iterator it = remainingInstances.begin();
         it != remainingInstances.end(); ++it)
    {
      assert(instances_[*it] != NULL);
      const Instance& instance = *instances_[*it];
        
      std::string sopInstanceUid;
      if (instance.HasPosition() &&
          instance.GetTags().LookupStringValue(
            sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
      {
        double p = LinearAlgebra::DotProduct(meanNormal, instance.GetPosition());
        items.push_back(SortableItem<float>(p, *it, sopInstanceUid));
      }
    }

    assert(items.size() <= n);
    
    std::sort(items.begin(), items.end());

    for (size_t i = 0; i < items.size(); i++)
    {
      AddFramesOfInstance(remainingInstances, items[i].GetInstanceIndex());
    }
  }


  size_t SortedFrames::GetFramesCount() const
  {
    if (sorted_)
    {
      return frames_.size();
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                      "Sort() has not been called");
    }
  }


  void SortedFrames::Sort()
  {
    if (!sorted_)
    {
      size_t totalFrames = 0;
      std::set<size_t> remainingInstances;
      
      for (size_t i = 0; i < instances_.size(); i++)
      {
        assert(instances_[i] != NULL);
        totalFrames += instances_[i]->GetNumberOfFrames();
        
        remainingInstances.insert(i);
      }

      frames_.clear();
      frames_.reserve(totalFrames);

      SortUsingIntegerTag(remainingInstances, Orthanc::DICOM_TAG_INSTANCE_NUMBER);  // VR is "IS"
      SortUsingIntegerTag(remainingInstances, Orthanc::DICOM_TAG_IMAGE_INDEX);  // VR is "US"
      SortUsing3DLocation(remainingInstances);
      SortUsingSopInstanceUid(remainingInstances);

      // The following could in theory happen if several instances
      // have the same SOPInstanceUID, no ordering is available
      for (std::set<size_t>::const_iterator it = remainingInstances.begin();
           it != remainingInstances.end(); it++)
      {
        AddFramesOfInstance(remainingInstances, *it);
      }

      if (frames_.size() != totalFrames ||
          !remainingInstances.empty())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      
      sorted_ = true;
    }
  }
}
