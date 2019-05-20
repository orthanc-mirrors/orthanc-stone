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

#include "ViewportController.h"

#include <Framework/StoneException.h>

#include <boost/make_shared.hpp>

using namespace Orthanc;

namespace OrthancStone
{
  ViewportController::ViewportController(MessageBroker& broker)
    : IObservable(broker)
  {
    scene_ = boost::make_shared<Scene2D>();
  }

  Scene2DPtr ViewportController::GetScene()
  {
    return scene_;
  }

  bool ViewportController::HandlePointerEvent(PointerEvent e)
  {
    throw StoneException(ErrorCode_NotImplemented);
  }

  std::vector<MeasureToolPtr> ViewportController::HitTestMeasureTools(
    ScenePoint2D p)
  {
    std::vector<MeasureToolPtr> ret;
    

    //for (size_t i = 0; i < measureTools_.size(); ++i)
    //{

    //}
    return ret;
  }

  const OrthancStone::AffineTransform2D& ViewportController::GetCanvasToSceneTransform() const
  {
    return scene_->GetCanvasToSceneTransform();
  }

  const OrthancStone::AffineTransform2D& ViewportController::GetSceneToCanvasTransform() const
  {
    return scene_->GetSceneToCanvasTransform();
  }

  void ViewportController::SetSceneToCanvasTransform(
    const AffineTransform2D& transform)
  {
    scene_->SetSceneToCanvasTransform(transform);
    BroadcastMessage(SceneTransformChanged(*this));
  }

  void ViewportController::FitContent(
    unsigned int canvasWidth, unsigned int canvasHeight)
  {
    scene_->FitContent(canvasWidth, canvasHeight);
    BroadcastMessage(SceneTransformChanged(*this));
  }

}

