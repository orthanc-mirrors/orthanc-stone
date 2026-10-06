/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "MagnifyingGlassTracker.h"

namespace OrthancStone
{
  void MagnifyingGlassTracker::Update(Scene2D& scene,
                                      const PointerEvent& event)
  {
    ScenePoint2D p = event.GetMainPosition().Apply(originalCanvasToScene_);

    scene.SetSceneToCanvasTransform(
      AffineTransform2D::Combine(
        originalSceneToCanvas_,
        AffineTransform2D::CreateOffset(p.GetX(), p.GetY()),
        AffineTransform2D::CreateScaling(5, 5),
        AffineTransform2D::CreateOffset(-pivot_.GetX(), -pivot_.GetY())));
  }
    

  MagnifyingGlassTracker::MagnifyingGlassTracker(Scene2D& scene,
                                                 const PointerEvent& event)
  {
    originalSceneToCanvas_ = scene.GetSceneToCanvasTransform();
    originalCanvasToScene_ = scene.GetCanvasToSceneTransform();
    pivot_ = event.GetMainPosition().Apply(scene.GetCanvasToSceneTransform());

    Update(scene, event);
  }
      

  void MagnifyingGlassTracker::PointerUp(const PointerEvent& event,
                                         Scene2D& scene)
  {
    Cancel(scene);
    OneGesturePointerTracker::PointerUp(event, scene);
  }

  
  bool MagnifyingGlassTracker::PointerMove(const PointerEvent& event,
                                           Scene2D& scene)
  {
    Update(scene, event);
    return true;
  }
    

  void MagnifyingGlassTracker::Cancel(Scene2D& scene)
  {
    scene.SetSceneToCanvasTransform(originalSceneToCanvas_);
  }
}
