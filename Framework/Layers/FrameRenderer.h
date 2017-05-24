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

#include "ILayerRenderer.h"

#include "../Toolbox/Slice.h"

namespace OrthancStone
{
  class FrameRenderer : public ILayerRenderer
  {
  private:
    SliceGeometry                 frameSlice_;
    double                        pixelSpacingX_;
    double                        pixelSpacingY_;
    RenderStyle                   style_;
    bool                          isFullQuality_;

    std::auto_ptr<CairoSurface>   display_;
    cairo_matrix_t                transform_;

  protected:
    virtual CairoSurface* GenerateDisplay(const RenderStyle& style) = 0;

  public:
    FrameRenderer(const SliceGeometry& frameSlice,
                  double pixelSpacingX,
                  double pixelSpacingY,
                  bool isFullQuality);

    // TODO Remove this overload
    static bool ComputeFrameExtent(double& x1,
                                   double& y1,
                                   double& x2,
                                   double& y2,
                                   const SliceGeometry& viewportSlice,
                                   const SliceGeometry& frameSlice,
                                   unsigned int frameWidth,
                                   unsigned int frameHeight,
                                   double pixelSpacingX,
                                   double pixelSpacingY);

    static bool ComputeFrameExtent(double& x1,
                                   double& y1,
                                   double& x2,
                                   double& y2,
                                   const SliceGeometry& viewportSlice,
                                   const Slice& slice)
    {
      return ComputeFrameExtent(x1, y1, x2, y2, viewportSlice,
                                slice.GetGeometry(), slice.GetWidth(), slice.GetHeight(),
                                slice.GetPixelSpacingX(), slice.GetPixelSpacingY());
    }

    virtual bool RenderLayer(CairoContext& context,
                             const ViewportGeometry& view,
                             const SliceGeometry& viewportSlice);

    virtual void SetLayerStyle(const RenderStyle& style);

    virtual bool IsFullQuality() 
    {
      return isFullQuality_;
    }

    // TODO Remove this overload
    static ILayerRenderer* CreateRenderer(Orthanc::ImageAccessor* frame,
                                          const SliceGeometry& frameSlice,
                                          const OrthancPlugins::IDicomDataset& dicom,
                                          double pixelSpacingX,
                                          double pixelSpacingY,
                                          bool isFullQuality);

    static ILayerRenderer* CreateRenderer(Orthanc::ImageAccessor* frame,
                                          const Slice& slice,
                                          bool isFullQuality);
  };
}
