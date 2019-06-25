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
    GetMeasureTool()->Disable();
    GetController()->RemoveMeasureTool(GetMeasureTool());
  }

  void CreateMeasureCommand::Redo()
  {
    GetMeasureTool()->Enable();
    GetController()->AddMeasureTool(GetMeasureTool());
  }

  CreateMeasureCommand::CreateMeasureCommand(boost::weak_ptr<ViewportController> controllerW)
    : TrackerCommand(controllerW)
  {

  }

  CreateMeasureCommand::~CreateMeasureCommand()
  {
    // deleting the command should not change the model state
    // we thus leave it as is
  }

  EditMeasureCommand::EditMeasureCommand(boost::shared_ptr<MeasureTool> measureTool, boost::weak_ptr<ViewportController> controllerW)
    : TrackerCommand(controllerW)
    , mementoOriginal_(measureTool->GetMemento())
    , mementoModified_(measureTool->GetMemento())
  {

  }

  EditMeasureCommand::~EditMeasureCommand()
  {

  }

  void EditMeasureCommand::Undo()
  {
    // simply disable the measure tool upon undo
    GetMeasureTool()->SetMemento(mementoOriginal_);
  }

  void EditMeasureCommand::Redo()
  {
    GetMeasureTool()->SetMemento(mementoModified_);
  }

  CreateLineMeasureCommand::CreateLineMeasureCommand(
    MessageBroker&         broker, 
    boost::weak_ptr<ViewportController> controllerW,
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

  EditLineMeasureCommand::EditLineMeasureCommand(
    boost::shared_ptr<LineMeasureTool>  measureTool,
    MessageBroker& broker,
    boost::weak_ptr<ViewportController> controllerW)
    : EditMeasureCommand(measureTool,controllerW)
    , measureTool_(measureTool)
  {
  }


  void EditLineMeasureCommand::SetStart(ScenePoint2D scenePos)
  {
    measureTool_->SetStart(scenePos);
    mementoModified_ = measureTool_->GetMemento();
  }


  void EditLineMeasureCommand::SetEnd(ScenePoint2D scenePos)
  {
    measureTool_->SetEnd(scenePos);
    mementoModified_ = measureTool_->GetMemento();
  }
    
  CreateAngleMeasureCommand::CreateAngleMeasureCommand(
    MessageBroker&         broker, 
    boost::weak_ptr<ViewportController> controllerW,
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

  boost::shared_ptr<ViewportController> TrackerCommand::GetController()
  {
    boost::shared_ptr<ViewportController> controller = controllerW_.lock();
    assert(controller); // accessing dead object?
    return controller;
  }

  EditAngleMeasureCommand::EditAngleMeasureCommand(
    boost::shared_ptr<AngleMeasureTool>  measureTool,
    MessageBroker& broker,
    boost::weak_ptr<ViewportController> controllerW)
    : EditMeasureCommand(measureTool, controllerW)
    , measureTool_(measureTool)
  {
  }

  void EditAngleMeasureCommand::SetCenter(ScenePoint2D scenePos)
  {
    measureTool_->SetCenter(scenePos);
    mementoModified_ = measureTool_->GetMemento();
  }


  void EditAngleMeasureCommand::SetSide1End(ScenePoint2D scenePos)
  {
    measureTool_->SetSide1End(scenePos);
    mementoModified_ = measureTool_->GetMemento();
  }


  void EditAngleMeasureCommand::SetSide2End(ScenePoint2D scenePos)
  {
    measureTool_->SetSide2End(scenePos);
    mementoModified_ = measureTool_->GetMemento();
  }

}
