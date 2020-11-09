/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "SortedFrames.h"

#include "GeometryToolbox.h"

#include <Logging.h>
#include <OrthancException.h>
#include <Toolbox.h>

namespace OrthancStone
{
  SortedFrames::Instance::Instance(const Orthanc::DicomMap& tags) :
    geometry_(tags)
  {
    tags_.Assign(tags);

    if (!tags.LookupStringValue(sopInstanceUid_, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    uint32_t tmp;
    if (tags.ParseUnsignedInteger32(tmp, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES) &&
        tmp > 0)
    {
      numberOfFrames_ = tmp;
    }
    else
    {
      numberOfFrames_ = 1;
    }

    std::string photometric;
    if (tags.LookupStringValue(photometric, Orthanc::DICOM_TAG_PHOTOMETRIC_INTERPRETATION, false))
    {
      Orthanc::Toolbox::StripSpaces(photometric);
      monochrome1_ = (photometric == "MONOCHROME1");
    }
    else
    {
      monochrome1_ = false;
    }

    bool ok = false;

    if (numberOfFrames_ > 1)
    {
      std::string offsets, increment;
      if (tags.LookupStringValue(offsets, Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR, false) &&
          tags.LookupStringValue(increment, Orthanc::DICOM_TAG_FRAME_INCREMENT_POINTER, false))
      {
        Orthanc::Toolbox::ToUpperCase(increment);
        if (increment != "3004,000C")
        {
          LOG(WARNING) << "Bad value for the FrameIncrementPointer tags in a multiframe image";
        }
        else if (LinearAlgebra::ParseVector(frameOffsets_, offsets))
        {
          if (frameOffsets_.size() == numberOfFrames_)
          {
            ok = true;
          }
          else
          {
            LOG(WARNING) << "The size of the GridFrameOffsetVector does not correspond to the number of frames";
          }
        }
        else
        {
          LOG(WARNING) << "Cannot parse the GridFrameOffsetVector tag";
        }
      }
      else
      {
        LOG(INFO) << "Missing the frame offset information in a multiframe image";
      }
    }

    if (!ok)
    {
      frameOffsets_.resize(numberOfFrames_);
      for (size_t i = 0; i < numberOfFrames_; i++)
      {
        frameOffsets_[i] = 0;
      }
    }
  }


  double SortedFrames::Instance::GetFrameOffset(unsigned int frame) const
  {
    assert(GetNumberOfFrames() == frameOffsets_.size());
    
    if (frame >= GetNumberOfFrames())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return frameOffsets_[frame];
    }
  }
  

  SortedFrames::Frame::Frame(const Instance& instance,
                             unsigned int frameNumber) :
    instance_(&instance),
    frameNumber_(frameNumber)
  {
    if (frameNumber >= instance.GetNumberOfFrames())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  const SortedFrames::Instance& SortedFrames::GetInstance(size_t instanceIndex) const
  {
    if (instanceIndex >= instances_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(instances_[instanceIndex] != NULL);
      return *instances_[instanceIndex];
    }
  }

  
  const SortedFrames::Frame& SortedFrames::GetFrame(size_t frameIndex) const
  {
    if (!sorted_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                      "Sort() has not been called");
    }
    if (frameIndex >= frames_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return frames_[frameIndex];
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

    instancesIndex_.clear();
    framesIndex_.clear();

    sorted_ = true;
  }


  void SortedFrames::AddInstance(const Orthanc::DicomMap& tags)
  {
    std::unique_ptr<Instance> instance(new Instance(tags));

    std::string studyInstanceUid, seriesInstanceUid, sopInstanceUid;
    if (!tags.LookupStringValue(studyInstanceUid, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) ||
        !tags.LookupStringValue(seriesInstanceUid, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false) ||
        !tags.LookupStringValue(sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
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

    if (instancesIndex_.find(sopInstanceUid) != instancesIndex_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                      "Cannot register twice the same SOP Instance UID");
    }

    instancesIndex_[sopInstanceUid] = instances_.size();

    instances_.push_back(instance.release());
    sorted_ = false;
    frames_.clear();
  }


  bool SortedFrames::LookupSopInstanceUid(size_t& instanceIndex,
                                          const std::string& sopInstanceUid) const
  {
    InstancesIndex::const_iterator found = instancesIndex_.find(sopInstanceUid);
    
    if (found == instancesIndex_.end())
    {
      return false;
    }
    else
    {
      instanceIndex = found->second;
      return true;
    }
  }

  
  void SortedFrames::AddFramesOfInstance(std::set<size_t>& remainingInstances,
                                         size_t instanceIndex)
  {
    assert(instances_[instanceIndex] != NULL);
    const Instance& instance = *instances_[instanceIndex];
    
    for (unsigned int i = 0; i < instance.GetNumberOfFrames(); i++)
    {
      framesIndex_[std::make_pair(instance.GetSopInstanceUid(), i)] = frames_.size();      
      frames_.push_back(Frame(instance, i));
    }

    assert(remainingInstances.find(instanceIndex) != remainingInstances.end());
    remainingInstances.erase(instanceIndex);
  }


  namespace
  {
    template<typename T>
    class SortableItem
    {
    private:
      T            value_;
      size_t       instanceIndex_;
      std::string  sopInstanceUid_;

    public:
      SortableItem(const T& value,
                   size_t instanceIndex,
                   const std::string& sopInstanceUid) :
        value_(value),
        instanceIndex_(instanceIndex),
        sopInstanceUid_(sopInstanceUid)
      {
      }

      size_t GetInstanceIndex() const
      {
        return instanceIndex_;
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

      if (instance.GetGeometry().IsValid())
      {
        n += 1;
        meanNormal += (instance.GetGeometry().GetNormal() - meanNormal) / static_cast<float>(n);
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
      if (instance.GetGeometry().IsValid() &&
          instance.GetTags().LookupStringValue(
            sopInstanceUid, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
      {
        double p = LinearAlgebra::DotProduct(meanNormal, instance.GetGeometry().GetOrigin());
        items.push_back(SortableItem<float>(static_cast<float>(p), *it, sopInstanceUid));
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


  CoordinateSystem3D SortedFrames::GetFrameGeometry(size_t frameIndex) const
  {
    const Frame& frame = GetFrame(frameIndex);
    CoordinateSystem3D geometry = frame.GetInstance().GetGeometry();

    if (geometry.IsValid())
    {
      geometry.SetOrigin(geometry.GetOrigin() + geometry.GetNormal() *
                         frame.GetInstance().GetFrameOffset(frame.GetFrameNumberInInstance()));
      return geometry;
    }
    else
    {
      return geometry;
    }
  }


  bool SortedFrames::LookupFrame(size_t& frameIndex,
                                 const std::string& sopInstanceUid,
                                 unsigned int frameNumber) const
  {
    if (sorted_)
    {
      FramesIndex::const_iterator found = framesIndex_.find(
        std::make_pair(sopInstanceUid, frameNumber));
      
      if (found == framesIndex_.end())
      {
        return false;
      }
      else
      {
        frameIndex = found->second;
        return true;
      }      
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
      framesIndex_.clear();

      SortUsingIntegerTag(remainingInstances, Orthanc::DICOM_TAG_INSTANCE_NUMBER);  // VR is "IS"
      SortUsingIntegerTag(remainingInstances, Orthanc::DICOM_TAG_IMAGE_INDEX);  // VR is "US"
      SortUsing3DLocation(remainingInstances);
      SortUsingSopInstanceUid(remainingInstances);

      // The following could in theory happen if several instances
      // have the same SOPInstanceUID, no ordering is available
      for (std::set<size_t>::const_iterator it = remainingInstances.begin();
           it != remainingInstances.end(); ++it)
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
