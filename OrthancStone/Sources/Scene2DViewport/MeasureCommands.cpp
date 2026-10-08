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

#include <memory>

#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace OrthancStone
{
  IViewport::ILock* MeasureCommand::GetViewportLock()
  {
    boost::shared_ptr<IViewport> viewport = viewport_.lock();
    if (viewport)
      return viewport->Lock();
    else
      return NULL;
  }


  void CreateMeasureCommand::Undo()
  {
    std::unique_ptr<IViewport::ILock> lock(GetViewportLock());
    // simply disable the measure tool upon undo
    measureTool_->Disable();
    lock->GetController().RemoveMeasureTool(measureTool_);
  }

  void CreateMeasureCommand::Redo()
  {
    std::unique_ptr<IViewport::ILock> lock(GetViewportLock());
    measureTool_->Enable();
    lock->GetController().AddMeasureTool(measureTool_);
  }

  CreateMeasureCommand::CreateMeasureCommand(const boost::shared_ptr<MeasureTool>& tool,
                                             boost::weak_ptr<IViewport> viewport) :
    MeasureCommand(viewport),
  measureTool_(tool)
  {

  }

  void DeleteMeasureCommand::Redo()
  {
    std::unique_ptr<IViewport::ILock> lock(GetViewportLock());
    // simply disable the measure tool upon undo
    measureTool_->Disable();
    lock->GetController().RemoveMeasureTool(measureTool_);
  }

  void DeleteMeasureCommand::Undo()
  {
    std::unique_ptr<IViewport::ILock> lock(GetViewportLock());
    measureTool_->Enable();
    lock->GetController().AddMeasureTool(measureTool_);
  }

  DeleteMeasureCommand::DeleteMeasureCommand(const boost::shared_ptr<MeasureTool>& measureTool,
                                             boost::weak_ptr<IViewport> viewport) :
    MeasureCommand(viewport),
    measureTool_(measureTool),
    mementoModified_(measureTool->CreateMemento()),
    mementoOriginal_(measureTool->CreateMemento())
  {
    if (!measureTool)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    {
      std::unique_ptr<IViewport::ILock> lock(GetViewportLock());
      measureTool_->Disable();
      lock->GetController().RemoveMeasureTool(measureTool_);
    }
  }

  EditMeasureCommand::EditMeasureCommand(const boost::shared_ptr<MeasureTool>& measureTool,
                                         boost::weak_ptr<IViewport> viewport) :
    MeasureCommand(viewport),
    measureTool_(measureTool),
    mementoModified_(measureTool->CreateMemento()),
    mementoOriginal_(measureTool->CreateMemento())
  {
    if (!measureTool)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }

  void EditMeasureCommand::Undo()
  {
    // simply disable the measure tool upon undo
    assert(mementoOriginal_.get() != NULL);
    GetMeasureTool().SetMemento(*mementoOriginal_);
  }

  void EditMeasureCommand::Redo()
  {
    assert(mementoModified_.get() != NULL);
    GetMeasureTool().SetMemento(*mementoModified_);
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

  const MeasureTool::IMemento& DeleteMeasureCommand::GetMementoOriginal() const
  {
    assert(mementoOriginal_.get() != NULL);
    return *mementoOriginal_;
  }    

  void DeleteMeasureCommand::SetMementoModified(MeasureTool::IMemento* memento)
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
}
