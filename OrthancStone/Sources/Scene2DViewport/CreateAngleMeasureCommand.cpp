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

#include "CreateAngleMeasureCommand.h"

#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace OrthancStone
{
  CreateAngleMeasureCommand::CreateAngleMeasureCommand(
    boost::shared_ptr<IViewport> viewport,
    ScenePoint2D           point)
    : CreateMeasureCommand(viewport)
    , measureTool_(AngleMeasureTool::Create(viewport))
  {
    
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();

    controller.AddMeasureTool(measureTool_);
    measureTool_->SetSide1End(point);
    measureTool_->SetCenter(point);
    measureTool_->SetSide2End(point);
  }

  /** This method sets center*/
  void CreateAngleMeasureCommand::SetCenter(ScenePoint2D scenePos)
  {
    measureTool_->SetCenter(scenePos);
  }

  /** This method sets end of side 2*/
  void CreateAngleMeasureCommand::SetSide2End(ScenePoint2D scenePos)
  {
    measureTool_->SetSide2End(scenePos);
  }
}