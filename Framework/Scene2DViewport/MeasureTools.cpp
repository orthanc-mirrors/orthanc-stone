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

#include "MeasureTools.h"

#include <Core/Logging.h>
#include <Core/Enumerations.h>
#include <Core>

#include <boost/math/constants/constants.hpp>

using namespace Orthanc;

namespace OrthancStone
{

  MeasureTool::~MeasureTool()
  {

  }

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

  OrthancStone::Scene2DPtr MeasureTool::GetScene()
  {
    Scene2DPtr scene = scene_.lock();
    if (!scene)
      throw OrthancException(ErrorCode_InternalError, "Using dead object!");
    return scene;
  }

  MeasureTool::MeasureTool(MessageBroker& broker, Scene2DWPtr sceneW)
    : IObserver(broker)
    , scene_(sceneW)
    , enabled_(true)
  {
    GetScene()->RegisterObserverCallback(
      new Callable<MeasureTool, Scene2D::SceneTransformChanged>
      (*this, &MeasureTool::OnSceneTransformChanged));
  }

  void MeasureTool::OnSceneTransformChanged(
    const Scene2D::SceneTransformChanged& message)
  {
    RefreshScene();
  }


}

