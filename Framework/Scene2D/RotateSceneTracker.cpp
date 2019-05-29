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

#include "RotateSceneTracker.h"
#include "../Scene2DViewport/ViewportController.h"

namespace OrthancStone
{
  RotateSceneTracker::RotateSceneTracker(boost::weak_ptr<ViewportController> controllerW,
                                         const PointerEvent& event)
    : OneGesturePointerTracker(controllerW)
    , click_(event.GetMainPosition())
    , aligner_(controllerW, click_)
    , isFirst_(true)
    , originalSceneToCanvas_(GetController()->GetSceneToCanvasTransform())
  {
  }
  
  void RotateSceneTracker::PointerMove(const PointerEvent& event)
  {
    ScenePoint2D p = event.GetMainPosition();
    double dx = p.GetX() - click_.GetX();
    double dy = p.GetY() - click_.GetY();

    if (std::abs(dx) > 5.0 || 
        std::abs(dy) > 5.0)
    {
      double a = atan2(dy, dx);

      if (isFirst_)
      {
        referenceAngle_ = a;
        isFirst_ = false;
      }

      GetController()->SetSceneToCanvasTransform(
        AffineTransform2D::Combine(
          AffineTransform2D::CreateRotation(a - referenceAngle_),
          originalSceneToCanvas_));
      
      aligner_.Apply();
    }
  }

  void RotateSceneTracker::Cancel()
  {
    GetController()->SetSceneToCanvasTransform(originalSceneToCanvas_);
  }

}
