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

#include "ILayerRendererFactory.h"
#include <IOrthancConnection.h>

namespace Deprecated
{
  class SingleFrameRendererFactory : public ILayerRendererFactory
  {
  private:
    OrthancPlugins::IOrthancConnection&           orthanc_;
    std::unique_ptr<OrthancPlugins::IDicomDataset>  dicom_;

    std::string           instance_;
    unsigned int          frame_;
    Orthanc::PixelFormat  format_;

  public:
    SingleFrameRendererFactory(OrthancPlugins::IOrthancConnection& orthanc,
                               const std::string& instanceId,
                               unsigned int frame);

    const OrthancPlugins::IDicomDataset& GetDataset() const
    {
      return *dicom_;
    }

    SliceGeometry GetSliceGeometry()
    {
      return SliceGeometry(*dicom_);
    }

    virtual bool GetExtent(double& x1,
                           double& y1,
                           double& x2,
                           double& y2,
                           const SliceGeometry& viewportSlice);

    virtual ILayerRenderer* CreateLayerRenderer(const SliceGeometry& viewportSlice);

    virtual bool HasSourceVolume() const
    {
      return false;
    }

    virtual ISliceableVolume& GetSourceVolume() const;
  };
}
