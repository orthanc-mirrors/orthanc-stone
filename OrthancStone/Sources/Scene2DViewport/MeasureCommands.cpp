/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "MeasureCommands.h"


namespace OrthancStone
{
  MeasureCommand::MeasureCommand(const boost::shared_ptr<MeasureTool>& tool) :
    measureTool_(tool)
  {
    if (!tool)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }



  void CreateMeasureCommand::Undo()
  {
    // simply disable the measure tool upon undo
    GetMeasureTool()->Disable();
    GetMeasureTool()->GetController().RemoveMeasureTool(GetMeasureTool());
  }


  void CreateMeasureCommand::Redo()
  {
    GetMeasureTool()->Enable();
    GetMeasureTool()->GetController().AddMeasureTool(GetMeasureTool());
  }




  EditMeasureCommand::EditMeasureCommand(const boost::shared_ptr<MeasureTool>& tool) :
    MeasureCommand(tool),
    mementoModified_(tool->CreateMemento()),
    mementoOriginal_(tool->CreateMemento())
  {
  }

  void EditMeasureCommand::Undo()
  {
    // simply disable the measure tool upon undo
    assert(mementoOriginal_.get() != NULL);
    GetMeasureTool()->SetMemento(*mementoOriginal_);
  }

  void EditMeasureCommand::Redo()
  {
    assert(mementoModified_.get() != NULL);
    GetMeasureTool()->SetMemento(*mementoModified_);
  }

  const MeasureTool::IMemento& EditMeasureCommand::GetMementoOriginal() const
  {
    assert(mementoOriginal_.get() != NULL);
    return *mementoOriginal_;
  }    

  void EditMeasureCommand::SetMementoModified(MeasureTool::IMemento* memento)
  {
    if (memento == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      mementoModified_.reset(memento);
    }
  }



  DeleteMeasureCommand::DeleteMeasureCommand(const boost::shared_ptr<MeasureTool>& tool) :
    MeasureCommand(tool)
  {
    GetMeasureTool()->Disable();
    GetMeasureTool()->GetController().RemoveMeasureTool(GetMeasureTool());  // TODO Refactoring - Should probably be moved into ViewportController
  }


  void DeleteMeasureCommand::Undo()
  {
    GetMeasureTool()->Enable();
    GetMeasureTool()->GetController().AddMeasureTool(GetMeasureTool());
  }


  void DeleteMeasureCommand::Redo()
  {
    // simply disable the measure tool upon undo
    GetMeasureTool()->Disable();
    GetMeasureTool()->GetController().RemoveMeasureTool(GetMeasureTool());
  }
}
