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


#pragma once

#include "MeasureTool.h"
#include "UndoStack.h"

#include <boost/shared_ptr.hpp>


namespace OrthancStone
{
  class MeasureCommand : public UndoStack::ICommand
  {
  private:
    boost::shared_ptr<MeasureTool>  measureTool_;

  protected:
    const boost::shared_ptr<MeasureTool>& GetMeasureTool() const
    {
      return measureTool_;
    }

  public:
    explicit MeasureCommand(const boost::shared_ptr<MeasureTool>& measureTool);

    virtual ~MeasureCommand()
    {
    }
    
    virtual void Undo() = 0;

    virtual void Redo() = 0;
  };

  
  class CreateMeasureCommand : public MeasureCommand
  {
  public:
    CreateMeasureCommand(const boost::shared_ptr<MeasureTool>& measureTool) :
      MeasureCommand(measureTool)
    {
    }

    virtual void Undo() ORTHANC_OVERRIDE;
    
    virtual void Redo() ORTHANC_OVERRIDE;
  };

  
  class EditMeasureCommand : public MeasureCommand
  {
  private:
    /** This memento is updated by the subclasses upon modifications */
    std::unique_ptr<MeasureTool::IMemento> mementoModified_;
    
    /** This memento is the original object state */
    std::unique_ptr<MeasureTool::IMemento> mementoOriginal_;

  public:
    EditMeasureCommand(const boost::shared_ptr<MeasureTool>& measureTool);

    virtual void Undo() ORTHANC_OVERRIDE;

    virtual void Redo() ORTHANC_OVERRIDE;

    const MeasureTool::IMemento& GetMementoOriginal() const;

    void SetMementoModified(MeasureTool::IMemento* memento);  // takes ownership
  };


  class DeleteMeasureCommand : public MeasureCommand
  {
  public:
    DeleteMeasureCommand(const boost::shared_ptr<MeasureTool>& measureTool);

    virtual void Undo() ORTHANC_OVERRIDE;
    
    virtual void Redo() ORTHANC_OVERRIDE;
  };
}
