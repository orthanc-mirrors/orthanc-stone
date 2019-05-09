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

namespace OrthancStone
{
  class TrackerCommand
  {
  public:
    TrackerCommand(Scene2D& scene) : scene_(scene)
    {

    }
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    virtual void Update(ScenePoint2D scenePos) = 0;

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
    CreateLineMeasureCommand::CreateLineMeasureCommand(
      Scene2D& scene, MeasureToolList& measureTools, ScenePoint2D point);
    
    void Update(ScenePoint2D scenePos) ORTHANC_OVERRIDE;

  private:
    virtual MeasureToolPtr GetMeasureTool() ORTHANC_OVERRIDE
    {
      return measureTool_;
    }
    LineMeasureToolPtr measureTool_;
    ScenePoint2D       start_;
    ScenePoint2D       end_;
  };

  typedef boost::shared_ptr<CreateLineMeasureCommand> CreateLineMeasureCommandPtr;
}

