/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include "MeasureCommands.h"

namespace OrthancStone
{
  class EditAngleMeasureCommand : public EditMeasureCommand
  {
  public:
    /** Ctor sets end of side 1*/
    EditAngleMeasureCommand(
      boost::shared_ptr<MeasureTool>  measureTool,
      boost::weak_ptr<IViewport> viewport);

    /** This method sets center*/
    void SetCenter(ScenePoint2D scenePos);

    /** This method sets end of side 1*/
    void SetSide1End(ScenePoint2D scenePos);

    /** This method sets end of side 2*/
    void SetSide2End(ScenePoint2D scenePos);

  private:
    virtual boost::shared_ptr<MeasureTool> GetMeasureTool() ORTHANC_OVERRIDE
    {
      return measureTool_;
    }
    boost::shared_ptr<MeasureTool> measureTool_;
  };
}
