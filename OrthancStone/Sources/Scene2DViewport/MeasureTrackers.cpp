/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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
#include <OrthancException.h>

namespace OrthancStone
{

  CreateMeasureTracker::CreateMeasureTracker(boost::shared_ptr<IViewport> viewport)
    : viewport_(viewport)
    , alive_(true)
    , commitResult_(true)
  {
  }

  void CreateMeasureTracker::Cancel()
  {
    commitResult_ = false;
    alive_ = false;
  }

  bool CreateMeasureTracker::IsAlive() const
  {
    return alive_;
  }

  CreateMeasureTracker::~CreateMeasureTracker()
  {
    // if the tracker completes successfully, we add the command
    // to the undo stack
    // otherwise, we simply undo it

    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());

    if (commitResult_)
      lock->GetController().PushCommand(command_);
    else
      command_->Undo();

    lock->Invalidate();
  }

  EditMeasureTracker::EditMeasureTracker(boost::shared_ptr<IViewport> viewport, const PointerEvent& e)
    : viewport_(viewport)
    , alive_(true)
    , commitResult_(true)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());

    originalClickPosition_ = e.GetMainPosition().Apply(
      lock->GetController().GetScene().GetCanvasToSceneTransform());
  }

  void EditMeasureTracker::Cancel()
  {
    commitResult_ = false;
    alive_ = false;
  }

  bool EditMeasureTracker::IsAlive() const
  {
    return alive_;
  }

  EditMeasureTracker::~EditMeasureTracker()
  {
    // if the tracker completes successfully, we add the command
    // to the undo stack
    // otherwise, we simply undo it

    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());

    if (commitResult_)
      lock->GetController().PushCommand(command_);
    else
      command_->Undo();

    lock->Invalidate();
  }
}


