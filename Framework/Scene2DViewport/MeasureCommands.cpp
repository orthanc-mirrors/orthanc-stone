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
    : MeasureCommand(controllerW)
  {

  }

  CreateMeasureCommand::~CreateMeasureCommand()
  {
    // deleting the command should not change the model state
    // we thus leave it as is
  }

  void DeleteMeasureCommand::Redo()
  {
    // simply disable the measure tool upon undo
    GetMeasureTool()->Disable();
    GetController()->RemoveMeasureTool(GetMeasureTool());
  }

  void DeleteMeasureCommand::Undo()
  {
    GetMeasureTool()->Enable();
    GetController()->AddMeasureTool(GetMeasureTool());
  }

  DeleteMeasureCommand::~DeleteMeasureCommand()
  {
    // deleting the command should not change the model state
    // we thus leave it as is
  }

  DeleteMeasureCommand::DeleteMeasureCommand(boost::shared_ptr<MeasureTool> measureTool, boost::weak_ptr<ViewportController> controllerW)
    : MeasureCommand(controllerW)
    , mementoOriginal_(measureTool->GetMemento())
    , measureTool_(measureTool)
    , mementoModified_(measureTool->GetMemento())
  {
    GetMeasureTool()->Disable();
    GetController()->RemoveMeasureTool(GetMeasureTool());
  }

  EditMeasureCommand::EditMeasureCommand(boost::shared_ptr<MeasureTool> measureTool, boost::weak_ptr<ViewportController> controllerW)
    : MeasureCommand(controllerW)
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

  boost::shared_ptr<ViewportController> MeasureCommand::GetController()
  {
    boost::shared_ptr<ViewportController> controller = controllerW_.lock();
    assert(controller); // accessing dead object?
    return controller;
  }
}
