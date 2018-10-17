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


#include "ViewportGeometry.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/math/special_functions/round.hpp>

namespace OrthancStone
{
  void ViewportGeometry::ComputeTransform()
  {
    // The following lines must be read in reverse order!
    cairo_matrix_t tmp;
    
    // Bring the center of the scene to the center of the view
    cairo_matrix_init_translate(&transform_, 
                                panX_ + static_cast<double>(width_) / 2.0, 
                                panY_ + static_cast<double>(height_) / 2.0);

    // Apply the zoom around (0,0)
    cairo_matrix_init_scale(&tmp, zoom_, zoom_);
    cairo_matrix_multiply(&transform_, &tmp, &transform_);

    // Bring the center of the scene to (0,0)
    cairo_matrix_init_translate(&tmp,
                                -(sceneExtent_.GetX1() + sceneExtent_.GetX2()) / 2.0,
                                -(sceneExtent_.GetY1() + sceneExtent_.GetY2()) / 2.0);
    cairo_matrix_multiply(&transform_, &tmp, &transform_);
  }


  ViewportGeometry::ViewportGeometry()
  {
    width_ = 0;
    height_ = 0;

    zoom_ = 1;
    panX_ = 0;
    panY_ = 0;

    ComputeTransform();
  }


  void ViewportGeometry::SetDisplaySize(unsigned int width,
                                        unsigned int height)
  {
    if (width_ != width ||
        height_ != height)
    {
      LOG(INFO) << "New display size: " << width << "x" << height;

      width_ = width;
      height_ = height;

      ComputeTransform();
    }
  }


  void ViewportGeometry::SetSceneExtent(const Extent2D& extent)
  {
    LOG(INFO) << "New scene extent: ("
              << extent.GetX1() << "," << extent.GetY1() << ") => ("
              << extent.GetX2() << "," << extent.GetY2() << ")";

    sceneExtent_ = extent;
    ComputeTransform();
  }


  void ViewportGeometry::MapDisplayToScene(double& sceneX /* out */,
                                           double& sceneY /* out */,
                                           double x,
                                           double y) const
  {
    cairo_matrix_t transform = transform_;

    if (cairo_matrix_invert(&transform) != CAIRO_STATUS_SUCCESS)
    {
      LOG(ERROR) << "Cannot invert singular matrix";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    sceneX = x;
    sceneY = y;
    cairo_matrix_transform_point(&transform, &sceneX, &sceneY);
  }


  void ViewportGeometry::MapSceneToDisplay(int& displayX /* out */,
                                           int& displayY /* out */,
                                           double x,
                                           double y) const
  {
    cairo_matrix_transform_point(&transform_, &x, &y);

    displayX = static_cast<int>(boost::math::iround(x));
    displayY = static_cast<int>(boost::math::iround(y));
  }


  void ViewportGeometry::FitContent()
  {
    if (width_ > 0 &&
        height_ > 0 &&
        !sceneExtent_.IsEmpty())
    {
      double zoomX = static_cast<double>(width_) / (sceneExtent_.GetX2() - sceneExtent_.GetX1());
      double zoomY = static_cast<double>(height_) / (sceneExtent_.GetY2() - sceneExtent_.GetY1());
      zoom_ = zoomX < zoomY ? zoomX : zoomY;

      panX_ = 0;
      panY_ = 0;

      ComputeTransform();
    }
  }


  void ViewportGeometry::ApplyTransform(CairoContext& context) const
  {
    cairo_set_matrix(context.GetObject(), &transform_);
  }


  void ViewportGeometry::GetPan(double& x,
                                double& y) const
  {
    x = panX_;
    y = panY_;
  }


  void ViewportGeometry::SetPan(double x,
                                double y)
  {
    panX_ = x;
    panY_ = y;
    ComputeTransform();
  }


  void ViewportGeometry::SetZoom(double zoom)
  {
    zoom_ = zoom;
    ComputeTransform();
  }
}
