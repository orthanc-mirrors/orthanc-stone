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

#include "../Scene2DViewport/OneGesturePointerTracker.h"

namespace OrthancStone
{
  class PanSceneTracker : public OneGesturePointerTracker
  {
  public:
    PanSceneTracker(boost::shared_ptr<IViewport> viewport,
                    const PointerEvent& event);

    virtual void PointerMove(const PointerEvent& event) ORTHANC_OVERRIDE;
    virtual void Cancel() ORTHANC_OVERRIDE;

  private:
    ScenePoint2D           pivot_;
    AffineTransform2D      originalSceneToCanvas_;
    AffineTransform2D      originalCanvasToScene_;
  };
}