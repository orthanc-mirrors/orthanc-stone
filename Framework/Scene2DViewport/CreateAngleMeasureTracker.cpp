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

#include "CreateAngleMeasureTracker.h"
#include <Core/OrthancException.h>

using namespace Orthanc;

namespace OrthancStone
{
  CreateAngleMeasureTracker::CreateAngleMeasureTracker(
    MessageBroker&                  broker,
    ViewportControllerWPtr          controllerW,
    std::vector<TrackerCommandPtr>& undoStack,
    MeasureToolList&                measureTools,
    const PointerEvent&             e)
    : CreateMeasureTracker(controllerW, undoStack, measureTools)
    , state_(CreatingSide1)
  {
    command_.reset(
      new CreateAngleMeasureCommand(
        broker,
        controllerW,
        measureTools,
        e.GetMainPosition().Apply(GetScene()->GetCanvasToSceneTransform())));
  }

  CreateAngleMeasureTracker::~CreateAngleMeasureTracker()
  {
  }

  void CreateAngleMeasureTracker::PointerMove(const PointerEvent& event)
  {
    assert(GetScene());

    if (!alive_)
    {
      throw OrthancException(ErrorCode_InternalError,
        "Internal error: wrong state in CreateAngleMeasureTracker::"
        "PointerMove: active_ == false");
    }

    ScenePoint2D scenePos = event.GetMainPosition().Apply(
      GetScene()->GetCanvasToSceneTransform());

    switch (state_)
    {
    case CreatingSide1:
      GetCommand()->SetCenter(scenePos);
      break;
    case CreatingSide2:
      GetCommand()->SetSide2End(scenePos);
      break;
    default:
      throw OrthancException(ErrorCode_InternalError,
        "Wrong state in CreateAngleMeasureTracker::"
        "PointerMove: state_ invalid");
    }
    //LOG(TRACE) << "scenePos.GetX() = " << scenePos.GetX() << "     " <<
    //  "scenePos.GetY() = " << scenePos.GetY();
  }

  void CreateAngleMeasureTracker::PointerUp(const PointerEvent& e)
  {
    // TODO: the current app does not prevent multiple PointerDown AND
    // PointerUp to be sent to the tracker.
    // Unless we augment the PointerEvent structure with the button index, 
    // we cannot really tell if this pointer up event matches the initial
    // pointer down event. Let's make it simple for now.

    switch (state_)
    {
    case CreatingSide1:
      state_ = CreatingSide2;
      break;
    case CreatingSide2:
      throw OrthancException(ErrorCode_InternalError,
        "Wrong state in CreateAngleMeasureTracker::"
        "PointerUp: state_ == CreatingSide2 ; this should not happen");
      break;
    default:
      throw OrthancException(ErrorCode_InternalError,
        "Wrong state in CreateAngleMeasureTracker::"
        "PointerMove: state_ invalid");
    }
  }

  void CreateAngleMeasureTracker::PointerDown(const PointerEvent& e)
  {
    switch (state_)
    {
    case CreatingSide1:
      throw OrthancException(ErrorCode_InternalError,
        "Wrong state in CreateAngleMeasureTracker::"
        "PointerDown: state_ == CreatingSide1 ; this should not happen");
      break;
    case CreatingSide2:
      // we are done
      alive_ = false;
      break;
    default:
      throw OrthancException(ErrorCode_InternalError,
        "Wrong state in CreateAngleMeasureTracker::"
        "PointerMove: state_ invalid");
    }
  }

  CreateAngleMeasureCommandPtr CreateAngleMeasureTracker::GetCommand()
  {
    return boost::dynamic_pointer_cast<CreateAngleMeasureCommand>(command_);
  }

}