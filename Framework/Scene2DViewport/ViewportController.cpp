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
#include "MeasureCommands.h"

#include "../StoneException.h"

#include <boost/make_shared.hpp>

namespace OrthancStone
{
  ViewportController::ViewportController(MessageBroker& broker)
    : IObservable(broker)
    , numAppliedCommands_(0)
    , canvasToSceneFactor_(0.0)
  {
    scene_ = boost::make_shared<Scene2D>();
  }

  boost::shared_ptr<const Scene2D> ViewportController::GetScene() const
  {
    return scene_;
  }

  boost::shared_ptr<Scene2D> ViewportController::GetScene()
  {
    return scene_;
  }

  bool ViewportController::HandlePointerEvent(PointerEvent e)
  {
    throw StoneException(ErrorCode_NotImplemented);
  }

  std::vector<boost::shared_ptr<MeasureTool>> ViewportController::HitTestMeasureTools(
    ScenePoint2D p)
  {
    std::vector<boost::shared_ptr<MeasureTool>> ret;
    
    for (size_t i = 0; i < measureTools_.size(); ++i)
    {
      if (measureTools_[i]->HitTest(p))
        ret.push_back(measureTools_[i]);
    }
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
    
    // update the canvas to scene factor
    canvasToSceneFactor_ = 0.0;
    canvasToSceneFactor_ = GetCanvasToSceneFactor();
  }

  void ViewportController::FitContent(
    unsigned int canvasWidth, unsigned int canvasHeight)
  {
    scene_->FitContent(canvasWidth, canvasHeight);
    BroadcastMessage(SceneTransformChanged(*this));
  }

  void ViewportController::PushCommand(boost::shared_ptr<TrackerCommand> command)
  {
    commandStack_.erase(
      commandStack_.begin() + numAppliedCommands_,
      commandStack_.end());
    
    ORTHANC_ASSERT(std::find(commandStack_.begin(), commandStack_.end(), command) 
      == commandStack_.end(), "Duplicate command");
    commandStack_.push_back(command);
    numAppliedCommands_++;
  }

  void ViewportController::Undo()
  {
    ORTHANC_ASSERT(CanUndo(), "");
    commandStack_[numAppliedCommands_-1]->Undo();
    numAppliedCommands_--;
  }

  void ViewportController::Redo()
  {
    ORTHANC_ASSERT(CanRedo(), "");
    commandStack_[numAppliedCommands_]->Redo();
    numAppliedCommands_++;
  }

  bool ViewportController::CanUndo() const
  {
    return numAppliedCommands_ > 0;
  }

  bool ViewportController::CanRedo() const
  {
    return numAppliedCommands_ < commandStack_.size();
  }

  void ViewportController::AddMeasureTool(boost::shared_ptr<MeasureTool> measureTool)
  {
    ORTHANC_ASSERT(std::find(measureTools_.begin(), measureTools_.end(), measureTool)
      == measureTools_.end(), "Duplicate measure tool");
    measureTools_.push_back(measureTool);
  }

  void ViewportController::RemoveMeasureTool(boost::shared_ptr<MeasureTool> measureTool)
  {
    ORTHANC_ASSERT(std::find(measureTools_.begin(), measureTools_.end(), measureTool)
      != measureTools_.end(), "Measure tool not found");
    measureTools_.erase(
      std::remove(measureTools_.begin(), measureTools_.end(), measureTool), 
      measureTools_.end());
  }


  double ViewportController::GetCanvasToSceneFactor() const
  {
    if (canvasToSceneFactor_ == 0)
    {
      canvasToSceneFactor_ =
        GetScene()->GetCanvasToSceneTransform().ComputeZoom();
    }
    return canvasToSceneFactor_;
  }

  double ViewportController::GetHandleSideLengthS() const
  {
    return HANDLE_SIDE_LENGTH_CANVAS_COORD * GetCanvasToSceneFactor();
  }

  double ViewportController::GetAngleToolArcRadiusS() const
  {
    return ARC_RADIUS_CANVAS_COORD * GetCanvasToSceneFactor();
  }

  double ViewportController::GetHitTestMaximumDistanceS() const
  {
    return HIT_TEST_MAX_DISTANCE_CANVAS_COORD * GetCanvasToSceneFactor();
  }

  double ViewportController::GetAngleTopTextLabelDistanceS() const
  {
    return TEXT_CENTER_DISTANCE_CANVAS_COORD * GetCanvasToSceneFactor();
  }
}

