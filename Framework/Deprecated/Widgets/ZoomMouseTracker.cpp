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


#include "ZoomMouseTracker.h"

#include <Logging.h>
#include <OrthancException.h>

namespace Deprecated
{
  ZoomMouseTracker::ZoomMouseTracker(WorldSceneWidget& that,
                                     int x,
                                     int y) :
    that_(that),
    originalZoom_(that.GetView().GetZoom()),
    downX_(x),
    downY_(y)
  {
    that.GetView().MapPixelCenterToScene(centerX_, centerY_, x, y);

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
    

  void ZoomMouseTracker::Render(OrthancStone::CairoContext& context,
                                double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void ZoomMouseTracker::MouseMove(int displayX,
                                   int displayY,
                                   double x,
                                   double y,
                                   const std::vector<Touch>& displayTouches,
                                   const std::vector<Touch>& sceneTouches)
  {
    static const double MIN_ZOOM = -4;
    static const double MAX_ZOOM = 4;

      
    if (!idle_)
    {
      double dy = static_cast<double>(displayY - downY_) * normalization_;  // In the range [-1,1]
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

      ViewportGeometry view = that_.GetView();
        
      view.SetZoom(z * originalZoom_);
        
      // Correct the pan so that the original click point is kept at
      // the same location on the display
      double panX, panY;
      view.GetPan(panX, panY);

      int tx, ty;
      view.MapSceneToDisplay(tx, ty, centerX_, centerY_);
      view.SetPan(panX + static_cast<double>(downX_ - tx),
                  panY + static_cast<double>(downY_ - ty));
        
      that_.SetView(view);
    }
  }
}
