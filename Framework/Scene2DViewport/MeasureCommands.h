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
#pragma once

#include "../Scene2D/Scene2D.h"

// to be moved into Stone
#include "PredeclaredTypes.h"
#include "MeasureTool.h"
#include "LineMeasureTool.h"
#include "AngleMeasureTool.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace OrthancStone
{
  class MeasureCommand : public boost::noncopyable
  {
  public:
    MeasureCommand(boost::weak_ptr<ViewportController> controllerW) 
      : controllerW_(controllerW)
    {

    }
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    
    virtual ~MeasureCommand() {};

  protected:
    boost::shared_ptr<ViewportController>  GetController();
    boost::weak_ptr<ViewportController> controllerW_;
  };

  class CreateMeasureCommand : public MeasureCommand
  {
  public:
    CreateMeasureCommand(boost::weak_ptr<ViewportController> controllerW);
    virtual ~CreateMeasureCommand();
    virtual void Undo() ORTHANC_OVERRIDE;
    virtual void Redo() ORTHANC_OVERRIDE;
  private:
    /** Must be implemented by the subclasses that create the actual tool */
    virtual boost::shared_ptr<MeasureTool> GetMeasureTool() = 0;
  };
  
  class EditMeasureCommand : public MeasureCommand
  {
  public:
    EditMeasureCommand(boost::shared_ptr<MeasureTool> measureTool, boost::weak_ptr<ViewportController> controllerW);
    virtual ~EditMeasureCommand();
    virtual void Undo() ORTHANC_OVERRIDE;
    virtual void Redo() ORTHANC_OVERRIDE;

    /** This memento is the original object state */
    boost::shared_ptr<MeasureToolMemento> mementoOriginal_;

  private:
    /** Must be implemented by the subclasses that edit the actual tool */
    virtual boost::shared_ptr<MeasureTool> GetMeasureTool() = 0;

  protected:

    /** This memento is updated by the subclasses upon modifications */
    boost::shared_ptr<MeasureToolMemento> mementoModified_;
  };

  class DeleteMeasureCommand : public MeasureCommand
  {
  public:
    DeleteMeasureCommand(boost::shared_ptr<MeasureTool> measureTool, boost::weak_ptr<ViewportController> controllerW);
    virtual ~DeleteMeasureCommand();
    virtual void Undo() ORTHANC_OVERRIDE;
    virtual void Redo() ORTHANC_OVERRIDE;

    /** This memento is the original object state */
    boost::shared_ptr<MeasureToolMemento> mementoOriginal_;

  private:
    /** Must be implemented by the subclasses that edit the actual tool */
    virtual boost::shared_ptr<MeasureTool> GetMeasureTool()
    {
      return measureTool_;
    }

    boost::shared_ptr<MeasureTool> measureTool_;

  protected:

    /** This memento is updated by the subclasses upon modifications */
    boost::shared_ptr<MeasureToolMemento> mementoModified_;
  };
}

