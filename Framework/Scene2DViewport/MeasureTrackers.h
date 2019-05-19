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

#include "IFlexiblePointerTracker.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/PointerEvent.h"

#include "MeasureTools.h"
#include "MeasureCommands.h"

#include <vector>

namespace OrthancStone
{
  class CreateMeasureTracker : public IFlexiblePointerTracker
  {
  public:
    virtual void Cancel() ORTHANC_OVERRIDE;
    virtual bool IsAlive() const ORTHANC_OVERRIDE;
  protected:
    CreateMeasureTracker(
      ViewportControllerWPtr          controllerW,
      std::vector<TrackerCommandPtr>& undoStack,
      std::vector<MeasureToolPtr>&    measureTools);

    ~CreateMeasureTracker();
  
  protected:
    CreateMeasureCommandPtr         command_;
    ViewportControllerWPtr          controllerW_;
    bool                            alive_;
    Scene2DPtr                      GetScene();

  private:
    std::vector<TrackerCommandPtr>& undoStack_;
    std::vector<MeasureToolPtr>&    measureTools_;
    bool                            commitResult_;
  };
}

