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


#pragma once

#include "CoordinateSystem3D.h"
#include "LinearAlgebra.h"

namespace OrthancStone
{
  class SortedFrames : public boost::noncopyable
  {
  private:
    class Instance : public boost::noncopyable
    {
    private:
      Orthanc::DicomMap   tags_;
      std::string         sopInstanceUid_;
      unsigned int        numberOfFrames_;
      CoordinateSystem3D  geometry_;
      bool                monochrome1_;

    public:
      explicit Instance(const Orthanc::DicomMap& tags);

      const Orthanc::DicomMap& GetTags() const
      {
        return tags_;
      }

      const std::string& GetSopInstanceUid() const
      {
        return sopInstanceUid_;
      }

      unsigned int GetNumberOfFrames() const
      {
        return numberOfFrames_;
      }

      const CoordinateSystem3D& GetGeometry() const
      {
        return geometry_;
      }

      bool IsMonochrome1() const
      {
        return monochrome1_;
      }
    };

    struct Frame
    {
    private:
      const Instance*  instance_;
      unsigned int     frameNumber_;

    public:
      Frame(const Instance& instance,
            unsigned int frameNumber);

      const Instance& GetInstance() const
      {
        return *instance_;
      }

      unsigned int GetFrameNumberInInstance() const
      {
        return frameNumber_;
      }
    };


    // Maps "SOPInstanceUID" to an index in "instances_"
    typedef std::map<std::string, size_t>  InstancesIndex;

    // Maps pair "(SOPInstanceUID, FrameNumber)" to an index in
    // "frames_" (only once "Sort()" is called)
    typedef std::map<std::pair<std::string, unsigned int>, size_t>  FramesIndex;

    std::string             studyInstanceUid_;
    std::string             seriesInstanceUid_;
    std::vector<Instance*>  instances_;
    std::vector<Frame>      frames_;
    bool                    sorted_;
    InstancesIndex          instancesIndex_;
    FramesIndex             framesIndex_;

    const Instance& GetInstance(size_t instanceIndex) const;

    const Frame& GetFrame(size_t frameIndex) const;

    void AddFramesOfInstance(std::set<size_t>& remainingInstances,
                             size_t instanceIndex);

    void SortUsingIntegerTag(std::set<size_t>& remainingInstances,
                             const Orthanc::DicomTag& tag);

    void SortUsingSopInstanceUid(std::set<size_t>& remainingInstances);

    void SortUsing3DLocation(std::set<size_t>& remainingInstances);

  public:
    SortedFrames() :
      sorted_(true)
    {
    }
  
    ~SortedFrames()
    {
      Clear();
    }
    
    void Clear();

    const std::string& GetStudyInstanceUid() const
    {
      return studyInstanceUid_;
    }

    const std::string& GetSeriesInstanceUid() const
    {
      return seriesInstanceUid_;
    }

    void AddInstance(const Orthanc::DicomMap& tags);

    size_t GetInstancesCount() const
    {
      return instances_.size();
    }

    const Orthanc::DicomMap& GetInstanceTags(size_t instanceIndex) const
    {
      return GetInstance(instanceIndex).GetTags();
    }

    const std::string& GetSopInstanceUid(size_t instanceIndex) const
    {
      return GetInstance(instanceIndex).GetSopInstanceUid();
    }

    const CoordinateSystem3D& GetInstanceGeometry(size_t instanceIndex) const
    {
      return GetInstance(instanceIndex).GetGeometry();
    }

    bool LookupSopInstanceUid(size_t& instanceIndex,
                              const std::string& sopInstanceUid) const;

    bool IsSorted() const
    {
      return sorted_;
    }

    size_t GetFramesCount() const;

    const Orthanc::DicomMap& GetFrameTags(size_t frameIndex) const
    {
      return GetFrame(frameIndex).GetInstance().GetTags();
    }

    const std::string& GetFrameSopInstanceUid(size_t frameIndex) const
    {
      return GetFrame(frameIndex).GetInstance().GetSopInstanceUid();
    }

    unsigned int GetFrameSiblingsCount(size_t frameIndex) const
    {
      return GetFrame(frameIndex).GetInstance().GetNumberOfFrames();
    }

    unsigned int GetFrameNumberInInstance(size_t frameIndex) const
    {
      return GetFrame(frameIndex).GetFrameNumberInInstance();
    }

    bool IsFrameMonochrome1(size_t frameIndex) const
    {
      return GetFrame(frameIndex).GetInstance().IsMonochrome1();
    }

    bool LookupFrame(size_t& frameIndex,
                     const std::string& sopInstanceUid,
                     unsigned int frameNumber) const;

    void Sort();
  };
}
