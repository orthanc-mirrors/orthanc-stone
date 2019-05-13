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

#include "CreateLineMeasureTracker.h"
#include <Core/OrthancException.h>

using namespace Orthanc;

namespace OrthancStone
{
  CreateLineMeasureTracker::CreateLineMeasureTracker(
    Scene2D&                        scene,
    std::vector<TrackerCommandPtr>& undoStack,
    std::vector<MeasureToolPtr>&    measureTools,
    const PointerEvent&             e)
    : CreateMeasureTracker(scene, undoStack, measureTools)
  {
    command_.reset(
      new CreateLineMeasureCommand(
        scene,
        measureTools,
        e.GetMainPosition().Apply(scene.GetCanvasToSceneTransform())));
  }

  CreateLineMeasureTracker::~CreateLineMeasureTracker()
  {

  }

  void CreateLineMeasureTracker::PointerMove(const PointerEvent& event)
  {
    if (!active_)
    {
      throw OrthancException(ErrorCode_InternalError,
        "Internal error: wrong state in CreateLineMeasureTracker::"
        "PointerMove: active_ == false");
    }

    ScenePoint2D scenePos = event.GetMainPosition().Apply(
      scene_.GetCanvasToSceneTransform());

    //LOG(TRACE) << "scenePos.GetX() = " << scenePos.GetX() << "     " <<
    //  "scenePos.GetY() = " << scenePos.GetY();

    CreateLineMeasureTracker* concreteThis =
      dynamic_cast<CreateLineMeasureTracker*>(this);
    assert(concreteThis != NULL);
    GetCommand()->SetEnd(scenePos);
  }

  void CreateLineMeasureTracker::PointerUp(const PointerEvent& e)
  {
    // TODO: the current app does not prevent multiple PointerDown AND
    // PointerUp to be sent to the tracker.
    // Unless we augment the PointerEvent structure with the button index, 
    // we cannot really tell if this pointer up event matches the initial
    // pointer down event. Let's make it simple for now.
    active_ = false;
  }

  void CreateLineMeasureTracker::PointerDown(const PointerEvent& e)
  {
    LOG(WARNING) << "Additional touches (fingers, pen, mouse buttons...) "
      "are ignored when the line measure creation tracker is active";
  }

  CreateLineMeasureCommandPtr CreateLineMeasureTracker::GetCommand()
  {
    return boost::dynamic_pointer_cast<CreateLineMeasureCommand>(command_);
  }

}
