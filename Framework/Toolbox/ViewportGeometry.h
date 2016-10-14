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


#pragma once

#include "../Viewport/CairoContext.h"

namespace OrthancStone
{
  // Not thread-safe
  class ViewportGeometry
  {
  private:
    // Extent of the scene (in world units)
    double   x1_;
    double   y1_;
    double   x2_;
    double   y2_;

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

    void SetSceneExtent(double x1,
                        double y1,
                        double x2,
                        double y2);

    void GetSceneExtent(double& x1,
                        double& y1,
                        double& x2,
                        double& y2) const;

    void MapDisplayToScene(double& sceneX /* out */,
                           double& sceneY /* out */,
                           double x,
                           double y) const;

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

    void SetDefaultView();

    void ApplyTransform(CairoContext& context) const;

    void GetPan(double& x,
                double& y) const;

    void SetPan(double x,
                double y);

    void SetZoom(double zoom);
  };
}
