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


#include "VolumeImageProgressivePolicy.h"

#include "../Toolbox/DownloadStack.h"
#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"

namespace OrthancStone
{
  class VolumeImageProgressivePolicy::AxialSlicesScheduler
  {
  private:
    size_t         depth_;
    DownloadStack  stack_;

  public:
    AxialSlicesScheduler(size_t depth) :
      depth_(depth),
      stack_(3 * depth)    // "3" stands for the number of quality levels
    {
      assert(depth > 0);
    }

    void TagFullPriority(int z,
                         int neighborhood)
    {
      DownloadStack::Writer writer(stack_);

      // Also schedule the neighboring slices for download in medium quality
      for (int offset = neighborhood; offset >= 1; offset--)
      {
        writer.SetTopNodePermissive((z + offset) + depth_ * Quality_Medium);
        writer.SetTopNodePermissive((z - offset) + depth_ * Quality_Medium);
      }

      writer.SetTopNodePermissive(z + depth_ * Quality_Full);
    }

    bool LookupSlice(unsigned int& z,
                     Quality& quality)
    {
      unsigned int value;
      if (stack_.Pop(value))
      {
        z = value % depth_;

        switch (value / depth_)
        {
          case 0:
            quality = Quality_Low;
            break;

          case 1:
            quality = Quality_Medium;
            break;

          case 2:
            quality = Quality_Full;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        return true;
      }
      else
      {
        return false;
      }
    }
  };


  bool VolumeImageProgressivePolicy::IsComplete()
  {
    boost::mutex::scoped_lock lock(qualityMutex_);
        
    for (size_t i = 0; i < axialSlicesQuality_.size(); i++)
    {
      if (axialSlicesQuality_[i] != Quality_Full)
      {
        return false;
      }
    }

    return true;
  }


  void VolumeImageProgressivePolicy::InitializeInternal(ImageBuffer3D& buffer,
                                                        ISeriesLoader& loader)
  {
    const size_t depth = loader.GetGeometry().GetSliceCount();

    isJpegAvailable_ = loader.IsJpegAvailable();

    axialSlicesQuality_.clear();
    axialSlicesQuality_.resize(depth, Quality_None);
    scheduler_.reset(new AxialSlicesScheduler(depth));
  }


  bool VolumeImageProgressivePolicy::DownloadStepInternal(bool& complete,
                                                          ImageBuffer3D& buffer,
                                                          ISeriesLoader& loader)
  {
    unsigned int z;
    Quality quality;

    if (!scheduler_->LookupSlice(z, quality))
    {
      // There is no more frame to be downloaded. Before stopping,
      // each loader thread checks whether all the frames have been
      // downloaded at maximum quality.
      complete = IsComplete();
      return true;
    }

    if (quality != Quality_Full &&
        !isJpegAvailable_)
    {
      // Cannot fulfill this command, as progressive JPEG download
      // is unavailable (i.e. the Web viewer plugin is unavailable)
      return false;
    }

    std::auto_ptr<Orthanc::ImageAccessor> frame;

    try
    {
      switch (quality)
      {
        case Quality_Low:
          frame.reset(loader.DownloadJpegFrame(z, 10));
          break;

        case Quality_Medium:
          frame.reset(loader.DownloadJpegFrame(z, 90));
          break;

        case Quality_Full:
          frame.reset(loader.DownloadFrame(z));
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
    catch (Orthanc::OrthancException&)
    {
      // The Orthanc server cannot decode this instance
      return false;
    }

    if (frame.get() != NULL)
    {
      boost::mutex::scoped_lock lock(qualityMutex_);

      if (axialSlicesQuality_[z] == Quality_None ||
          axialSlicesQuality_[z] < quality)
      {
        axialSlicesQuality_[z] = quality;

        ImageBuffer3D::SliceWriter writer(buffer, VolumeProjection_Axial, z);
        Orthanc::ImageProcessing::Convert(writer.GetAccessor(), *frame);
      }
    }

    return false;
  }


  bool VolumeImageProgressivePolicy::IsFullQualityAxial(size_t slice)
  {
    scheduler_->TagFullPriority(slice, 3);

    {
      boost::mutex::scoped_lock lock(qualityMutex_);
      return (axialSlicesQuality_[slice] == Quality_Full);
    }
  }
}
