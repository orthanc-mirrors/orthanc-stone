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


#include "PanZoomMouseTracker.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <math.h>

namespace Deprecated
{
  Touch GetCenter(const std::vector<Touch>& touches)
  {
    return Touch((touches[0].x + touches[1].x) / 2.0f, (touches[0].y + touches[1].y) / 2.0f);
  }

  double GetDistance(const std::vector<Touch>& touches)
  {
    float dx = touches[0].x - touches[1].x;
    float dy = touches[0].y - touches[1].y;
    return sqrt((double)(dx * dx) + (double)(dy * dy));
  }


  PanZoomMouseTracker::PanZoomMouseTracker(WorldSceneWidget& that,
                                           const std::vector<Touch>& startTouches)
    : that_(that),
      originalZoom_(that.GetView().GetZoom())
  {
    that.GetView().GetPan(originalPanX_, originalPanY_);
    that.GetView().MapPixelCenterToScene(originalSceneTouches_, startTouches);

    originalDisplayCenter_ = GetCenter(startTouches);
    originalSceneCenter_ = GetCenter(originalSceneTouches_);
    originalDisplayDistanceBetweenTouches_ = GetDistance(startTouches);

//    printf("original Pan %f %f\n", originalPanX_, originalPanY_);
//    printf("original Zoom %f \n", originalZoom_);
//    printf("original distance %f \n", (float)originalDisplayDistanceBetweenTouches_);
//    printf("original display touches 0 %f %f\n", startTouches[0].x, startTouches[0].y);
//    printf("original display touches 1 %f %f\n", startTouches[1].x, startTouches[1].y);
//    printf("original Scene center %f %f\n", originalSceneCenter_.x, originalSceneCenter_.y);

    unsigned int height = that.GetView().GetDisplayHeight();

    if (height <= 3)
    {
      idle_ = true;
      LOG(WARNING) << "image is too small to zoom (current height = " << height << ")";
    }
    else
    {
      idle_ = false;
      normalization_ = 1.0 / static_cast<double>(height - 1);
    }

  }


  void PanZoomMouseTracker::Render(OrthancStone::CairoContext& context,
                                   double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void PanZoomMouseTracker::MouseMove(int displayX,
                                      int displayY,
                                      double x,
                                      double y,
                                      const std::vector<Touch>& displayTouches,
                                      const std::vector<Touch>& sceneTouches)
  {
    ViewportGeometry view = that_.GetView();

//    printf("Display touches 0 %f %f\n", displayTouches[0].x, displayTouches[0].y);
//    printf("Display touches 1 %f %f\n", displayTouches[1].x, displayTouches[1].y);
//    printf("Scene touches 0 %f %f\n", sceneTouches[0].x, sceneTouches[0].y);
//    printf("Scene touches 1 %f %f\n", sceneTouches[1].x, sceneTouches[1].y);

//    printf("zoom = %f\n", view.GetZoom());
    Touch currentSceneCenter = GetCenter(sceneTouches);
    double panX = originalPanX_ + (currentSceneCenter.x - originalSceneCenter_.x) * view.GetZoom();
    double panY = originalPanY_ + (currentSceneCenter.y - originalSceneCenter_.y) * view.GetZoom();

    view.SetPan(panX, panY);

    static const double MIN_ZOOM = -4;
    static const double MAX_ZOOM = 4;

    if (!idle_)
    {
      double currentDistanceBetweenTouches = GetDistance(displayTouches);

      double dy = static_cast<double>(currentDistanceBetweenTouches - originalDisplayDistanceBetweenTouches_) * normalization_;  // In the range [-1,1]
      double z;

      // Linear interpolation from [-1, 1] to [MIN_ZOOM, MAX_ZOOM]
      if (dy < -1.0)
      {
        z = MIN_ZOOM;
      }
      else if (dy > 1.0)
      {
        z = MAX_ZOOM;
      }
      else
      {
        z = MIN_ZOOM + (MAX_ZOOM - MIN_ZOOM) * (dy + 1.0) / 2.0;
      }

      z = pow(2.0, z);

      view.SetZoom(z * originalZoom_);
    }

    that_.SetView(view);
  }
}
