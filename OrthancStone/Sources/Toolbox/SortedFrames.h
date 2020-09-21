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


#pragma once

#include "LinearAlgebra.h"

namespace OrthancStone
{
  class SortedFrames : public boost::noncopyable
  {
  private:
    class Instance : public boost::noncopyable
    {
    private:
      bool               hasPosition_;
      Orthanc::DicomMap  tags_;
      std::string        sopInstanceUid_;
      unsigned int       numberOfFrames_;
      Vector             normal_;    // Only used in "Sort()"
      Vector             position_;  // Only used in "Sort()"
      bool               monochrome1_;

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

      bool HasPosition() const
      {
        return hasPosition_;
      }

      const Vector& GetNormal() const;

      const Vector& GetPosition() const;

      bool IsMonochrome1() const
      {
        return monochrome1_;
      }
    };

    struct Frame
    {
    private:
      const Instance*  instance_;
      unsigned int     frameIndex_;

    public:
      Frame(const Instance& instance,
            unsigned int frameIndex);

      const Instance& GetInstance() const
      {
        return *instance_;
      }

      unsigned int GetFrameIndex() const
      {
        return frameIndex_;
      }
    };

    std::string             studyInstanceUid_;
    std::string             seriesInstanceUid_;
    std::vector<Instance*>  instances_;
    std::vector<Frame>      frames_;
    bool                    sorted_;

    const Instance& GetInstance(size_t index) const;

    const Frame& GetFrame(size_t index) const;

    void AddFramesOfInstance(std::set<size_t>& remainingInstances,
                             size_t index);

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

    const Orthanc::DicomMap& GetInstanceTags(size_t index) const
    {
      return GetInstance(index).GetTags();
    }

    const std::string& GetSopInstanceUid(size_t index) const
    {
      return GetInstance(index).GetSopInstanceUid();
    }

    bool IsSorted() const
    {
      return sorted_;
    }

    size_t GetFramesCount() const;

    const Orthanc::DicomMap& GetFrameTags(size_t index) const
    {
      return GetFrame(index).GetInstance().GetTags();
    }

    const std::string& GetFrameSopInstanceUid(size_t index) const
    {
      return GetFrame(index).GetInstance().GetSopInstanceUid();
    }

    unsigned int GetFrameSiblingsCount(size_t index) const
    {
      return GetFrame(index).GetInstance().GetNumberOfFrames();
    }

    unsigned int GetFrameIndex(size_t index) const
    {
      return GetFrame(index).GetFrameIndex();
    }

    bool IsFrameMonochrome1(size_t index) const
    {
      return GetFrame(index).GetInstance().IsMonochrome1();
    }

    void Sort();
  };
}
