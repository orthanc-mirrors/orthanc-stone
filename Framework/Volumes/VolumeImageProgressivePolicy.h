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


#pragma once

#include "VolumeImagePolicyBase.h"

namespace OrthancStone
{
  class VolumeImageProgressivePolicy : public VolumeImagePolicyBase
  {
  private:
    enum Quality
    {
      Quality_Low = 0,
      Quality_Medium = 1,
      Quality_Full = 2,
      Quality_None = 3
    };

    class AxialSlicesScheduler;

    std::auto_ptr<AxialSlicesScheduler>   scheduler_;
    boost::mutex                          qualityMutex_;
    std::vector<Quality>                  axialSlicesQuality_;
    bool                                  isJpegAvailable_;

    bool IsComplete();

  protected:
    virtual void InitializeInternal(ImageBuffer3D& buffer,
                                    ISeriesLoader& loader);

    virtual bool DownloadStepInternal(bool& complete,
                                      ImageBuffer3D& buffer,
                                      ISeriesLoader& loader);

  public:
    virtual bool IsFullQualityAxial(size_t slice);
  };
}
