/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "VolumeImage.h"

namespace OrthancStone
{
  class VolumeImagePolicyBase : public VolumeImage::IDownloadPolicy
  {
  private:
    ImageBuffer3D*  buffer_;
    ISeriesLoader*  loader_;

  protected:
    virtual void InitializeInternal(ImageBuffer3D& buffer,
                                    ISeriesLoader& loader) = 0;

    virtual bool DownloadStepInternal(bool& complete,
                                      ImageBuffer3D& buffer,
                                      ISeriesLoader& loader) = 0;

  public:
    VolumeImagePolicyBase();

    virtual void Initialize(ImageBuffer3D& buffer,
                            ISeriesLoader& loader);

    virtual void Finalize()
    {
    }

    virtual bool DownloadStep(bool& complete);
  };
}
