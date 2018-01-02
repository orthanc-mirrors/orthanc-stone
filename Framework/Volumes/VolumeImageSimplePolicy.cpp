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


#include "VolumeImageSimplePolicy.h"

#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"

namespace OrthancStone
{
  void VolumeImageSimplePolicy::InitializeInternal(ImageBuffer3D& buffer,
                                                   ISeriesLoader& loader)
  {
    boost::mutex::scoped_lock  lock(mutex_);

    const size_t depth = loader.GetGeometry().GetSliceCount();
    pendingSlices_.clear();

    for (size_t i = 0; i < depth; i++)
    {
      pendingSlices_.insert(i);
    }

    doneSlices_.clear();
    doneSlices_.resize(depth, false);
  }


  bool VolumeImageSimplePolicy::DownloadStepInternal(bool& complete,
                                                     ImageBuffer3D& buffer,
                                                     ISeriesLoader& loader)
  {
    size_t slice;

    {
      boost::mutex::scoped_lock  lock(mutex_);
      
      if (pendingSlices_.empty())
      {
        return true;
      }
      else
      {
        slice = *pendingSlices_.begin();
        pendingSlices_.erase(slice);
      }
    }

    std::auto_ptr<Orthanc::ImageAccessor> frame;

    try
    {
      frame.reset(loader.DownloadFrame(slice));
    }
    catch (Orthanc::OrthancException&)
    {
      // The Orthanc server cannot decode this instance
      return false;
    }

    if (frame.get() != NULL)
    {
      {
        ImageBuffer3D::SliceWriter writer(buffer, VolumeProjection_Axial, slice);
        Orthanc::ImageProcessing::Convert(writer.GetAccessor(), *frame);
      }

      {
        boost::mutex::scoped_lock  lock(mutex_);

        doneSlices_[slice] = true;

        if (pendingSlices_.empty())
        {
          complete = true;
          return true;
        }
      }
    }
      
    return false;
  }


  bool VolumeImageSimplePolicy::IsFullQualityAxial(size_t slice)
  {
    boost::mutex::scoped_lock  lock(mutex_);
    return doneSlices_[slice];
  }
}
