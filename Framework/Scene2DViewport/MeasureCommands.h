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
  class TrackerCommand : public boost::noncopyable
  {
  public:
    TrackerCommand(boost::weak_ptr<ViewportController> controllerW) 
      : controllerW_(controllerW)
    {

    }
    virtual void Undo() = 0;
    virtual void Redo() = 0;

  protected:
    boost::shared_ptr<ViewportController>  GetController();
    boost::weak_ptr<ViewportController> controllerW_;
  };

  class CreateMeasureCommand : public TrackerCommand
  {
  public:
    CreateMeasureCommand(boost::weak_ptr<ViewportController> controllerW);
    ~CreateMeasureCommand();
    virtual void Undo() ORTHANC_OVERRIDE;
    virtual void Redo() ORTHANC_OVERRIDE;
  private:
    /** Must be implemented by the subclasses that create the actual tool */
    virtual boost::shared_ptr<MeasureTool> GetMeasureTool() = 0;
  };
  
  class CreateLineMeasureCommand : public CreateMeasureCommand
  {
  public:
    CreateLineMeasureCommand(
      MessageBroker&         broker, 
      boost::weak_ptr<ViewportController> controllerW,
      ScenePoint2D           point);
    
    // the starting position is set in the ctor
    void SetEnd(ScenePoint2D scenePos);

  private:
    virtual boost::shared_ptr<MeasureTool> GetMeasureTool() ORTHANC_OVERRIDE
    {
      return measureTool_;
    }
    boost::shared_ptr<LineMeasureTool> measureTool_;
  };


  class CreateAngleMeasureCommand : public CreateMeasureCommand
  {
  public:
    /** Ctor sets end of side 1*/
    CreateAngleMeasureCommand(
      MessageBroker&         broker, 
      boost::weak_ptr<ViewportController> controllerW,
      ScenePoint2D           point);

    /** This method sets center*/
    void SetCenter(ScenePoint2D scenePos);

    /** This method sets end of side 2*/
    void SetSide2End(ScenePoint2D scenePos);

  private:
    virtual boost::shared_ptr<MeasureTool> GetMeasureTool() ORTHANC_OVERRIDE
    {
      return measureTool_;
    }
    boost::shared_ptr<AngleMeasureTool> measureTool_;
  };

}

