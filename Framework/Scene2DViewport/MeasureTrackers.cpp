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

namespace OrthancStone
{

  CreateMeasureTracker::CreateMeasureTracker(boost::weak_ptr<ViewportController> controllerW)
    : controllerW_(controllerW)
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
    if (commitResult_)
      controllerW_.lock()->PushCommand(command_);
    else
      command_->Undo();
  }

  boost::shared_ptr<Scene2D> CreateMeasureTracker::GetScene()
  {
    return controllerW_.lock()->GetScene();
  }

}


