/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "../Toolbox/ISeriesLoader.h"

namespace OrthancStone
{
  class SeriesFrameRendererFactory : public ILayerRendererFactory
  {
  private:
    std::auto_ptr<ISeriesLoader>  loader_;
    size_t                        currentFrame_;
    bool                          fast_;

    std::auto_ptr<OrthancPlugins::IDicomDataset>  currentDataset_;

    void ReadCurrentFrameDataset(size_t frame);

    void GetCurrentPixelSpacing(double& spacingX,
                                double& spacingY) const;

    double GetCurrentSliceThickness() const;

  public:
    SeriesFrameRendererFactory(ISeriesLoader* loader,   // Takes ownership
                               bool fast);

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
