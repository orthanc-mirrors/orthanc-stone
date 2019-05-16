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

#include <Framework/Scene2D/Scene2D.h>
#include <boost/shared_ptr.hpp>

// to be moved into Stone
#include "MeasureTools.h"
#include "LineMeasureTool.h"
#include "AngleMeasureTool.h"

namespace OrthancStone
{
  //class LineMeasureTool;
  //typedef boost::shared_ptr<LineMeasureTool> LineMeasureToolPtr;
  //class AngleMeasureTool;
  //typedef boost::shared_ptr<AngleMeasureTool> AngleMeasureToolPtr;
   
  class TrackerCommand
  {
  public:
    TrackerCommand(Scene2D& scene) : scene_(scene)
    {

    }
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    Scene2D& GetScene()
    {
      return scene_;
    }

  protected:
    Scene2D& scene_;
  private:
    TrackerCommand(const TrackerCommand&);
    TrackerCommand& operator=(const TrackerCommand&);
  };

  typedef boost::shared_ptr<TrackerCommand> TrackerCommandPtr;
  
  class CreateMeasureCommand : public TrackerCommand
  {
  public:
    CreateMeasureCommand(Scene2D& scene, MeasureToolList& measureTools);
    ~CreateMeasureCommand();
    virtual void Undo() ORTHANC_OVERRIDE;
    virtual void Redo() ORTHANC_OVERRIDE;
  protected:
    MeasureToolList& measureTools_;
  private:
    /** Must be implemented by the subclasses that create the actual tool */
    virtual MeasureToolPtr GetMeasureTool() = 0;
  };

  typedef boost::shared_ptr<CreateMeasureCommand> CreateMeasureCommandPtr;

  class CreateLineMeasureCommand : public CreateMeasureCommand
  {
  public:
    CreateLineMeasureCommand(
      MessageBroker& broker, Scene2D& scene, MeasureToolList& measureTools, ScenePoint2D point);
    
    // the starting position is set in the ctor
    void SetEnd(ScenePoint2D scenePos);

  private:
    virtual MeasureToolPtr GetMeasureTool() ORTHANC_OVERRIDE
    {
      return measureTool_;
    }
    LineMeasureToolPtr measureTool_;
  };

  typedef boost::shared_ptr<CreateLineMeasureCommand> CreateLineMeasureCommandPtr;

  class CreateAngleMeasureCommand : public CreateMeasureCommand
  {
  public:
    /** Ctor sets end of side 1*/
    CreateAngleMeasureCommand(
      MessageBroker& broker, Scene2D& scene, MeasureToolList& measureTools, ScenePoint2D point);

    /** This method sets center*/
    void SetCenter(ScenePoint2D scenePos);

    /** This method sets end of side 2*/
    void SetSide2End(ScenePoint2D scenePos);

  private:
    virtual MeasureToolPtr GetMeasureTool() ORTHANC_OVERRIDE
    {
      return measureTool_;
    }
    AngleMeasureToolPtr measureTool_;
  };

  typedef boost::shared_ptr<CreateAngleMeasureCommand> CreateAngleMeasureCommandPtr;
}

