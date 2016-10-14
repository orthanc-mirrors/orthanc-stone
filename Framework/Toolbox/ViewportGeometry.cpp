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


#include "ViewportGeometry.h"

#include "../Orthanc/Core/Logging.h"
#include "../Orthanc/Core/OrthancException.h"

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
    cairo_matrix_init_translate(&tmp, -(x1_ + x2_) / 2.0, -(y1_ + y2_) / 2.0);
    cairo_matrix_multiply(&transform_, &tmp, &transform_);
  }


  ViewportGeometry::ViewportGeometry()
  {
    x1_ = 0;
    y1_ = 0;
    x2_ = 0;
    y2_ = 0;

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


  void ViewportGeometry::SetSceneExtent(double x1,
                                        double y1,
                                        double x2,
                                        double y2)
  {
    if (x1 == x1_ &&
        y1 == y1_ &&
        x2 == x2_ &&
        y2 == y2_)
    {
      return;
    }
    else if (x1 > x2 || 
             y1 > y2)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      LOG(INFO) << "New scene extent: (" << x1 << "," << y1 << ") => (" << x2 << "," << y2 << ")";

      x1_ = x1;
      y1_ = y1;
      x2_ = x2;
      y2_ = y2;

      ComputeTransform();
    }
  }


  void ViewportGeometry::GetSceneExtent(double& x1,
                                        double& y1,
                                        double& x2,
                                        double& y2) const
  {
    x1 = x1_;
    y1 = y1_;
    x2 = x2_;
    y2 = y2_;
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


  void ViewportGeometry::SetDefaultView()
  {
    if (width_ > 0 &&
        height_ > 0 &&
        x2_ > x1_ + 10 * std::numeric_limits<double>::epsilon() &&
        y2_ > y1_ + 10 * std::numeric_limits<double>::epsilon())
    {
      double zoomX = static_cast<double>(width_) / (x2_ - x1_);
      double zoomY = static_cast<double>(height_) / (y2_ - y1_);
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
