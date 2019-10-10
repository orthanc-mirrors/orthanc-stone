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

#include "EditAngleMeasureTracker.h"
#include "EditAngleMeasureCommand.h"

#include "../StoneException.h"

namespace OrthancStone
{
  EditAngleMeasureTracker::EditAngleMeasureTracker(
    boost::shared_ptr<AngleMeasureTool>  measureTool,
    MessageBroker& broker,
    boost::weak_ptr<ViewportController> controllerW,
    const PointerEvent& e)
    : EditMeasureTracker(controllerW, e)
  {
    ScenePoint2D scenePos = e.GetMainPosition().Apply(
      GetScene().GetCanvasToSceneTransform());

    modifiedZone_ = measureTool->AngleHitTest(scenePos);

    command_.reset(new EditAngleMeasureCommand(measureTool, broker, controllerW));
  }

  EditAngleMeasureTracker::~EditAngleMeasureTracker()
  {

  }

  void EditAngleMeasureTracker::PointerMove(const PointerEvent& e)
  {
    ScenePoint2D scenePos = e.GetMainPosition().Apply(
      GetScene().GetCanvasToSceneTransform());

    ScenePoint2D delta = scenePos - GetOriginalClickPosition();

    boost::shared_ptr<AngleMeasureToolMemento> memento =
      boost::dynamic_pointer_cast<AngleMeasureToolMemento>(command_->mementoOriginal_);

    ORTHANC_ASSERT(memento.get() != NULL);

    switch (modifiedZone_)
    {
    case AngleMeasureTool::AngleHighlightArea_Center:
    {
      ScenePoint2D newCenter = memento->center_ + delta;
      GetCommand()->SetCenter(newCenter);
    }
    break;
    case AngleMeasureTool::AngleHighlightArea_Side1:
    case AngleMeasureTool::AngleHighlightArea_Side2:
    {
      ScenePoint2D newCenter = memento->center_ + delta;
      ScenePoint2D newSide1End = memento->side1End_ + delta;
      ScenePoint2D newSide2End = memento->side2End_ + delta;
      GetCommand()->SetCenter(newCenter);
      GetCommand()->SetSide1End(newSide1End);
      GetCommand()->SetSide2End(newSide2End);
    }
    break;
    case AngleMeasureTool::AngleHighlightArea_Side1End:
    {
      ScenePoint2D newSide1End = memento->side1End_ + delta;
      GetCommand()->SetSide1End(newSide1End);
    }
    break;
    case AngleMeasureTool::AngleHighlightArea_Side2End:
    {
      ScenePoint2D newSide2End = memento->side2End_ + delta;
      GetCommand()->SetSide2End(newSide2End);
    }
    break;
    default:
      LOG(WARNING) << "Warning: please retry the measuring tool editing operation!";
      break;
    }
  }

  void EditAngleMeasureTracker::PointerUp(const PointerEvent& e)
  {
    alive_ = false;
  }

  void EditAngleMeasureTracker::PointerDown(const PointerEvent& e)
  {
    LOG(WARNING) << "Additional touches (fingers, pen, mouse buttons...) "
      "are ignored when the edit angle tracker is active";
  }

  boost::shared_ptr<EditAngleMeasureCommand> EditAngleMeasureTracker::GetCommand()
  {
    boost::shared_ptr<EditAngleMeasureCommand> ret = boost::dynamic_pointer_cast<EditAngleMeasureCommand>(command_);
    ORTHANC_ASSERT(ret.get() != NULL, "Internal error in EditAngleMeasureTracker::GetCommand()");
    return ret;
  }

}
