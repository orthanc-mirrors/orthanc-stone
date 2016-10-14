/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "VolumeImageSimplePolicy.h"

#include "../Orthanc/Core/Images/ImageProcessing.h"

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
