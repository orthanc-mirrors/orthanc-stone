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
  MeasureCommand::MeasureCommand(ViewportController& controller,
                                 const boost::shared_ptr<MeasureTool>& tool) :
    controller_(controller),
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
    GetController().RemoveMeasureTool(GetMeasureTool());
  }


  void CreateMeasureCommand::Redo()
  {
    GetMeasureTool()->Enable();
    GetController().AddMeasureTool(GetMeasureTool());
  }




  EditMeasureCommand::EditMeasureCommand(ViewportController& controller,
                                         const boost::shared_ptr<MeasureTool>& tool) :
    MeasureCommand(controller, tool),
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



  DeleteMeasureCommand::DeleteMeasureCommand(ViewportController& controller,
                                             const boost::shared_ptr<MeasureTool>& tool) :
    MeasureCommand(controller, tool)
  {
    GetMeasureTool()->Disable();
    GetController().RemoveMeasureTool(GetMeasureTool());  // TODO Refactoring - Should probably be moved into ViewportController
  }


  void DeleteMeasureCommand::Undo()
  {
    GetMeasureTool()->Enable();
    GetController().AddMeasureTool(GetMeasureTool());
  }


  void DeleteMeasureCommand::Redo()
  {
    // simply disable the measure tool upon undo
    GetMeasureTool()->Disable();
    GetController().RemoveMeasureTool(GetMeasureTool());
  }
}
