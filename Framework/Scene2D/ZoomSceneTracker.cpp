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


#include "ZoomSceneTracker.h"
#include "../Scene2DViewport/ViewportController.h"

using boost::weak_ptr;
using boost::shared_ptr;

namespace OrthancStone
{
  ZoomSceneTracker::ZoomSceneTracker(weak_ptr<ViewportController> controllerW,
                                     const PointerEvent& event,
                                     unsigned int canvasHeight)
    : OneGesturePointerTracker(controllerW)
    , clickY_(event.GetMainPosition().GetY())
    , aligner_(controllerW, event.GetMainPosition())
    , originalSceneToCanvas_(GetController()->GetSceneToCanvasTransform())
  {
    if (canvasHeight <= 3)
    {
      active_ = false;
    }
    else
    {
      normalization_ = 1.0 / static_cast<double>(canvasHeight - 1);
      active_ = true;
    }
  }
  
  void ZoomSceneTracker::PointerMove(const PointerEvent& event)
  {
    static const double MIN_ZOOM = -4;
    static const double MAX_ZOOM = 4;
      
    if (active_)
    {
      double y = event.GetMainPosition().GetY();
      
      // In the range [-1,1]
      double dy = static_cast<double>(y - clickY_) * normalization_;  
      
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

      double zoom = pow(2.0, z);

      // The controller is a weak pointer. It could be deleted when the
      // tracker is still alive (for instance, because of a lost WebGL
      // context)
      if(GetController().get() != NULL)
      {
        GetController()->SetSceneToCanvasTransform(
          AffineTransform2D::Combine(
            AffineTransform2D::CreateScaling(zoom, zoom),
            originalSceneToCanvas_));

        aligner_.Apply();
      }
    }
  }

  void ZoomSceneTracker::Cancel()
  {
      if(GetController().get() != NULL)
      {
        GetController()->SetSceneToCanvasTransform(originalSceneToCanvas_);
      }
  }
}
