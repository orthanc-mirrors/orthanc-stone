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

#include "MeasureTool.h"

#include <Core/Logging.h>
#include <Core/Enumerations.h>
#include <Core/OrthancException.h>

#include <boost/math/constants/constants.hpp>

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


  boost::shared_ptr<const ViewportController> MeasureTool::GetController() const
  {
    boost::shared_ptr<const ViewportController> controller = controllerW_.lock();
    if (!controller)
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError,
        "Using dead ViewportController object!");
    return controller;
  }

  boost::shared_ptr<ViewportController> MeasureTool::GetController()
  {
#if 1
    return boost::const_pointer_cast<ViewportController>
      (const_cast<const MeasureTool*>(this)->GetController());
    //return boost::const_<boost::shared_ptr<ViewportController>>
    //  (const_cast<const MeasureTool*>(this)->GetController());
#else
    boost::shared_ptr<ViewportController> controller = controllerW_.lock();
    if (!controller)
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError, 
        "Using dead ViewportController object!");
    return controller;
#endif
  }

  MeasureTool::MeasureTool(
    boost::weak_ptr<ViewportController> controllerW)
    : controllerW_(controllerW)
    , enabled_(true)
  {
    // TODO => Move this out of constructor
    Register<ViewportController::SceneTransformChanged>(*GetController(), &MeasureTool::OnSceneTransformChanged);
  }


  bool MeasureTool::IsSceneAlive() const
  {
    boost::shared_ptr<ViewportController> controller = controllerW_.lock();
    return (controller.get() != NULL);
  }

  void MeasureTool::OnSceneTransformChanged(
    const ViewportController::SceneTransformChanged& message)
  {
    RefreshScene();
  }


}

