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

#include "MeasureTool.h"

#include <Logging.h>
#include <Enumerations.h>
#include <OrthancException.h>

#include <boost/math/constants/constants.hpp>

#include "../Viewport/IViewport.h"

namespace OrthancStone
{
  void MeasureTool::Enable()
  {
    enabled_ = true;
    RefreshScene();
  }

  void MeasureTool::Disable()
  {
    enabled_ = false;
    RefreshScene();
  }

  bool MeasureTool::IsEnabled() const
  {
    return enabled_;
  }

  MeasureTool::MeasureTool(
    boost::shared_ptr<IViewport> viewport)
    : viewport_(viewport)
    , enabled_(true)
  {

  }

  void MeasureTool::PostConstructor()
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();

    Register<ViewportController::SceneTransformChanged>(
      controller, 
      &MeasureTool::OnSceneTransformChanged);
  }

  bool MeasureTool::IsSceneAlive() const
  {
    // since the lifetimes of the viewport, viewportcontroller (and the
    // measuring tools inside it) are linked, the scene is always alive as 
    // long as "this" is alive
    return true;
  }

  void MeasureTool::OnSceneTransformChanged(
    const ViewportController::SceneTransformChanged& message)
  {
    RefreshScene();
  }


}
