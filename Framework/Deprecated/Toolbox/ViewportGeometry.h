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

#include "../../Viewport/CairoContext.h"
#include "../../Toolbox/Extent2D.h"
#include "../../Toolbox/LinearAlgebra.h"
#include "../Viewport/IMouseTracker.h"  // to include "Touch" definition

namespace Deprecated
{
  class ViewportGeometry
  {
  private:
    // Extent of the scene (in world units)
    OrthancStone::Extent2D   sceneExtent_;

    // Size of the display (in pixels)
    unsigned int  width_;
    unsigned int  height_;

    // Zoom/pan
    double   zoom_;
    double   panX_;  // In pixels (display units)
    double   panY_;

    cairo_matrix_t  transform_;  // Scene-to-display transformation

    void ComputeTransform();

  public:
    ViewportGeometry();

    void SetDisplaySize(unsigned int width,
                        unsigned int height);

    void SetSceneExtent(const OrthancStone::Extent2D& extent);

    const OrthancStone::Extent2D& GetSceneExtent() const
    {
      return sceneExtent_;
    }

    void MapDisplayToScene(double& sceneX /* out */,
                           double& sceneY /* out */,
                           double x,
                           double y) const;

    void MapPixelCenterToScene(double& sceneX /* out */,
                               double& sceneY /* out */,
                               int x,
                               int y) const;

    void MapPixelCenterToScene(std::vector<Touch>& sceneTouches /* out */,
                               const std::vector<Touch>& displayTouches) const;

    void MapSceneToDisplay(int& displayX /* out */,
                           int& displayY /* out */,
                           double x,
                           double y) const;

    unsigned int GetDisplayWidth() const
    {
      return width_;
    }

    unsigned int GetDisplayHeight() const
    {
      return height_;
    }

    double GetZoom() const
    {
      return zoom_;
    }

    void FitContent();

    void ApplyTransform(OrthancStone::CairoContext& context) const;

    void GetPan(double& x,
                double& y) const;

    void SetPan(double x,
                double y);

    void SetZoom(double zoom);

    OrthancStone::Matrix GetMatrix() const;
  };
}