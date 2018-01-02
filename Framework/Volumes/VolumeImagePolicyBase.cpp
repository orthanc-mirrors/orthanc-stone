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


#include "VolumeImagePolicyBase.h"

namespace OrthancStone
{
  VolumeImagePolicyBase::VolumeImagePolicyBase() : 
    buffer_(NULL),
    loader_(NULL)
  {
  }


  void VolumeImagePolicyBase::Initialize(ImageBuffer3D& buffer,
                                         ISeriesLoader& loader)
  {
    if (buffer_ != NULL ||
        loader_ != NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    buffer_ = &buffer;
    loader_ = &loader;

    InitializeInternal(buffer, loader);
  }


  bool VolumeImagePolicyBase::DownloadStep(bool& complete)
  {
    if (buffer_ == NULL ||
        loader_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return DownloadStepInternal(complete, *buffer_, *loader_);                                    
    }
  }
}
