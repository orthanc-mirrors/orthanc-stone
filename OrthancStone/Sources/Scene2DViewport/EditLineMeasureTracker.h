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

#include "MeasureTrackers.h"

namespace OrthancStone
{
  class EditLineMeasureCommand;

  class EditLineMeasureTracker : public EditMeasureTracker
  {
  public:
    /**
    When you create this tracker, you need to supply it with the undo stack
    where it will store the commands that perform the actual measure tool
    creation and modification.
    In turn, a container for these commands to store the actual measuring
    must be supplied, too
    */
    EditLineMeasureTracker(
      boost::shared_ptr<MeasureTool>  measureTool,
      boost::shared_ptr<IViewport> viewport,
      const PointerEvent&                 e);

    ~EditLineMeasureTracker();

    virtual void PointerMove(const PointerEvent& e) ORTHANC_OVERRIDE;
    virtual void PointerUp(const PointerEvent& e) ORTHANC_OVERRIDE;
    virtual void PointerDown(const PointerEvent& e) ORTHANC_OVERRIDE;

  private:
    LineMeasureTool::LineHighlightArea modifiedZone_;

    boost::shared_ptr<EditLineMeasureCommand> GetCommand();
  };
}