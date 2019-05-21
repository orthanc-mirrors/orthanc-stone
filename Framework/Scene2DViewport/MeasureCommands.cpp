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

#include "MeasureCommands.h"

#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace OrthancStone
{
  void CreateMeasureCommand::Undo()
  {
    // simply disable the measure tool upon undo
    GetController()->RemoveMeasureTool(GetMeasureTool());
  }

  void CreateMeasureCommand::Redo()
  {
    GetController()->AddMeasureTool(GetMeasureTool());
  }

  CreateMeasureCommand::CreateMeasureCommand(ViewportControllerWPtr controllerW)
    : TrackerCommand(controllerW)
  {

  }

  CreateMeasureCommand::~CreateMeasureCommand()
  {
    // deleting the command should not change the model state
    // we thus leave it as is
  }

  CreateLineMeasureCommand::CreateLineMeasureCommand(
    MessageBroker&         broker, 
    ViewportControllerWPtr controllerW,
    ScenePoint2D           point)
    : CreateMeasureCommand(controllerW)
    , measureTool_(
        boost::make_shared<LineMeasureTool>(boost::ref(broker), controllerW))
  {
    GetController()->AddMeasureTool(measureTool_);
    measureTool_->Set(point, point);
  }

  void CreateLineMeasureCommand::SetEnd(ScenePoint2D scenePos)
  {
    measureTool_->SetEnd(scenePos);
  }

  CreateAngleMeasureCommand::CreateAngleMeasureCommand(
    MessageBroker&         broker, 
    ViewportControllerWPtr controllerW,
    ScenePoint2D           point)
    : CreateMeasureCommand(controllerW)
    , measureTool_(
      boost::make_shared<AngleMeasureTool>(boost::ref(broker), controllerW))
  {
    GetController()->AddMeasureTool(measureTool_);
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

  ViewportControllerPtr TrackerCommand::GetController()
  {
    ViewportControllerPtr controller = controllerW_.lock();
    assert(controller); // accessing dead object?
    return controller;
  }
}
