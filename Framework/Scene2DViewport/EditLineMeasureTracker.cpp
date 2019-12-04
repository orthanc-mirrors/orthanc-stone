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

#include "EditLineMeasureTracker.h"
#include "EditLineMeasureCommand.h"

#include "../StoneException.h"


namespace OrthancStone
{
  EditLineMeasureTracker::EditLineMeasureTracker(
    boost::shared_ptr<MeasureTool>  measureTool,
    boost::weak_ptr<ViewportController> controllerW,
    const PointerEvent& e) 
    : EditMeasureTracker(controllerW, e)
  {
    ScenePoint2D scenePos = e.GetMainPosition();

    {
      boost::shared_ptr<ViewportController> controller = controllerW.lock();
      if (controller)
      {
        scenePos = e.GetMainPosition().Apply(controller->GetScene().GetCanvasToSceneTransform());
      }
    }

    modifiedZone_ = dynamic_cast<LineMeasureTool&>(*measureTool).LineHitTest(scenePos);

    command_.reset(new EditLineMeasureCommand(measureTool, controllerW));
  }

  EditLineMeasureTracker::~EditLineMeasureTracker()
  {

  }

  void EditLineMeasureTracker::PointerMove(const PointerEvent& e)
  {
    boost::shared_ptr<ViewportController> controller = controllerW_.lock();
    if (controller)
    {
      ScenePoint2D scenePos = e.GetMainPosition().Apply(
        controller->GetScene().GetCanvasToSceneTransform());
      
      ScenePoint2D delta = scenePos - GetOriginalClickPosition();

      boost::shared_ptr<LineMeasureToolMemento> memento =
        boost::dynamic_pointer_cast<LineMeasureToolMemento>(command_->mementoOriginal_);

      ORTHANC_ASSERT(memento.get() != NULL);

      switch (modifiedZone_)
      {
        case LineMeasureTool::LineHighlightArea_Start:
        {
          ScenePoint2D newStart = memento->start_ + delta;
          GetCommand()->SetStart(newStart);
        }
        break;
        case LineMeasureTool::LineHighlightArea_End:
        {
          ScenePoint2D newEnd = memento->end_ + delta;
          GetCommand()->SetEnd(newEnd);
        }
        break;
        case LineMeasureTool::LineHighlightArea_Segment:
        {
          ScenePoint2D newStart = memento->start_ + delta;
          ScenePoint2D newEnd = memento->end_ + delta;
          GetCommand()->SetStart(newStart);
          GetCommand()->SetEnd(newEnd);
        }
        break;
        default:
          LOG(WARNING) << "Warning: please retry the measuring tool editing operation!";
          break;
      }
    }
  }
  
  void EditLineMeasureTracker::PointerUp(const PointerEvent& e)
  {
    alive_ = false;
  }

  void EditLineMeasureTracker::PointerDown(const PointerEvent& e)
  {
    LOG(WARNING) << "Additional touches (fingers, pen, mouse buttons...) "
      "are ignored when the edit line tracker is active";
  }

  boost::shared_ptr<EditLineMeasureCommand> EditLineMeasureTracker::GetCommand()
  {
    boost::shared_ptr<EditLineMeasureCommand> ret = boost::dynamic_pointer_cast<EditLineMeasureCommand>(command_);
    ORTHANC_ASSERT(ret.get() != NULL, "Internal error in EditLineMeasureTracker::GetCommand()");
    return ret;
  }

}
