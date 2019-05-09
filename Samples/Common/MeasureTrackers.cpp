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

#include "MeasureTrackers.h"
#include <Core/OrthancException.h>

using namespace Orthanc;

namespace OrthancStone
{

  CreateMeasureTracker::CreateMeasureTracker(
    Scene2D&                        scene,
    std::vector<TrackerCommandPtr>& undoStack,
    std::vector<MeasureToolPtr>&    measureTools)
    : scene_(scene)
    , undoStack_(undoStack)
    , measureTools_(measureTools)
    , commitResult_(true)
  {
  }

  CreateMeasureTracker::~CreateMeasureTracker()
  {
    // if the tracker completes successfully, we add the command
    // to the undo stack

    // otherwise, we simply undo it
    if (commitResult_)
      undoStack_.push_back(command_);
    else
      command_->Undo();
  }
  
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

  void CreateMeasureTracker::Update(const PointerEvent& event)
  {
    ScenePoint2D scenePos = event.GetMainPosition().Apply(
      scene_.GetCanvasToSceneTransform());
    
    LOG(TRACE) << "scenePos.GetX() = " << scenePos.GetX() << "     " <<
      "scenePos.GetY() = " << scenePos.GetY();

    CreateLineMeasureTracker* concreteThis =
      dynamic_cast<CreateLineMeasureTracker*>(this);
    assert(concreteThis != NULL);
    command_->Update(scenePos);
  }

  void CreateMeasureTracker::Release()
  {
    commitResult_ = false;
  }

}


