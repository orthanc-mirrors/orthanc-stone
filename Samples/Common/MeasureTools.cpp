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

#include <boost/math/constants/constants.hpp>

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

  OrthancStone::Scene2D& MeasureTool::GetScene()
  {
    return scene_;
  }

  MeasureTool::MeasureTool(MessageBroker& broker, Scene2D& scene)
    : IObserver(broker)
    , scene_(scene)
    , enabled_(true)
  {
    scene_.RegisterObserverCallback(
      new Callable<MeasureTool, Scene2D::SceneTransformChanged>
      (*this, &MeasureTool::OnSceneTransformChanged));
  }

  void MeasureTool::OnSceneTransformChanged(
    const Scene2D::SceneTransformChanged& message)
  {
    RefreshScene();
  }


}
