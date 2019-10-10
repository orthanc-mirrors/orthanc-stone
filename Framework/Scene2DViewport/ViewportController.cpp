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

#include "UndoStack.h"
#include "MeasureCommands.h"

#include "../StoneException.h"

#include <boost/make_shared.hpp>

namespace OrthancStone
{
  ViewportController::ViewportController(boost::weak_ptr<UndoStack> undoStackW,
                                         MessageBroker& broker,
                                         IViewport& viewport)
    : IObservable(broker)
    , undoStackW_(undoStackW)
    , canvasToSceneFactor_(0.0)
    , viewport_(viewport)
  {
  }
 
  ViewportController::~ViewportController()
  {

  }

  boost::shared_ptr<UndoStack> ViewportController::GetUndoStack()
  {
    return undoStackW_.lock();
  }

  boost::shared_ptr<const UndoStack> ViewportController::GetUndoStack() const
  {
    return undoStackW_.lock();
  }

  void ViewportController::PushCommand(boost::shared_ptr<MeasureCommand> command)
  {
    boost::shared_ptr<UndoStack> undoStack = undoStackW_.lock();
    if(undoStack.get() != NULL)
      undoStack->PushCommand(command);
    else
    {
      LOG(ERROR) << "Internal error: no undo stack in the viewport controller!";
    }
  }

  void ViewportController::Undo()
  {
    boost::shared_ptr<UndoStack> undoStack = undoStackW_.lock();
    if (undoStack.get() != NULL)
      undoStack->Undo();
    else
    {
      LOG(ERROR) << "Internal error: no undo stack in the viewport controller!";
    }
  }

  void ViewportController::Redo()
  {
    boost::shared_ptr<UndoStack> undoStack = undoStackW_.lock();
    if (undoStack.get() != NULL)
      undoStack->Redo();
    else
    {
      LOG(ERROR) << "Internal error: no undo stack in the viewport controller!";
    }
  }

  bool ViewportController::CanUndo() const
  {
    boost::shared_ptr<UndoStack> undoStack = undoStackW_.lock();
    if (undoStack.get() != NULL)
      return undoStack->CanUndo();
    else
    {
      LOG(ERROR) << "Internal error: no undo stack in the viewport controller!";
      return false;
    }
  }

  bool ViewportController::CanRedo() const
  {
    boost::shared_ptr<UndoStack> undoStack = undoStackW_.lock();
    if (undoStack.get() != NULL)
      return undoStack->CanRedo();
    else
    {
      LOG(ERROR) << "Internal error: no undo stack in the viewport controller!";
      return false;
    }
  }
  
  bool ViewportController::HandlePointerEvent(PointerEvent e)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }

  std::vector<boost::shared_ptr<MeasureTool> > ViewportController::HitTestMeasureTools(
    ScenePoint2D p)
  {
    std::vector<boost::shared_ptr<MeasureTool> > ret;
    
    for (size_t i = 0; i < measureTools_.size(); ++i)
    {
      if (measureTools_[i]->HitTest(p))
        ret.push_back(measureTools_[i]);
    }
    return ret;
  }


  void ViewportController::ResetMeasuringToolsHighlight()
  {
    for (size_t i = 0; i < measureTools_.size(); ++i)
    {
      measureTools_[i]->ResetHighlightState();
    }
  }

  const OrthancStone::AffineTransform2D& ViewportController::GetCanvasToSceneTransform() const
  {
    return GetScene().GetCanvasToSceneTransform();
  }

  const OrthancStone::AffineTransform2D& ViewportController::GetSceneToCanvasTransform() const
  {
    return GetScene().GetSceneToCanvasTransform();
  }

  void ViewportController::SetSceneToCanvasTransform(
    const AffineTransform2D& transform)
  {
    viewport_.GetScene().SetSceneToCanvasTransform(transform);
    BroadcastMessage(SceneTransformChanged(*this));
    
    // update the canvas to scene factor
    canvasToSceneFactor_ = 0.0;
    canvasToSceneFactor_ = GetCanvasToSceneFactor();
  }

  void ViewportController::FitContent(
    unsigned int canvasWidth, unsigned int canvasHeight)
  {
    viewport_.GetScene().FitContent(canvasWidth, canvasHeight);
    BroadcastMessage(SceneTransformChanged(*this));
  }

  void ViewportController::FitContent()
  {
    if (viewport_.HasCompositor())
    {
      const ICompositor& compositor = viewport_.GetCompositor();
      viewport_.GetScene().FitContent(compositor.GetCanvasWidth(), compositor.GetCanvasHeight());
      BroadcastMessage(SceneTransformChanged(*this));
    }
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
        GetScene().GetCanvasToSceneTransform().ComputeZoom();
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
