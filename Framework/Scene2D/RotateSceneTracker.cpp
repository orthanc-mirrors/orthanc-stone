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

namespace OrthancStone
{
  RotateSceneTracker::RotateSceneTracker(Scene2D& scene,
                                         const PointerEvent& event,
                                         unsigned int canvasWidth,
                                         unsigned int canvasHeight) :
    scene_(scene),
    click_(event.GetMainPosition()),
    aligner_(scene, click_, canvasWidth, canvasHeight),
    isFirst_(true),
    originalSceneToCanvas_(scene.GetSceneToCanvasTransform())
  {
  }


  void RotateSceneTracker::Update(const PointerEvent& event)
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

      scene_.SetSceneToCanvasTransform(
        AffineTransform2D::Combine(
          AffineTransform2D::CreateRotation(a - referenceAngle_),
          originalSceneToCanvas_));
      
      aligner_.Apply();
    }
  }
}
