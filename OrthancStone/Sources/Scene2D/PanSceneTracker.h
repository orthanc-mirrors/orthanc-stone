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

#include "../Scene2DViewport/OneGesturePointerTracker.h"

namespace OrthancStone
{
  class PanSceneTracker : public OneGesturePointerTracker
  {
  private:
    boost::weak_ptr<IViewport> viewport_;
    ScenePoint2D               pivot_;
    AffineTransform2D          originalSceneToCanvas_;
    AffineTransform2D          originalCanvasToScene_;

  public:
    PanSceneTracker(boost::weak_ptr<IViewport> viewport,
                    const PointerEvent& event);

    virtual void PointerMove(const PointerEvent& event,
                             const Scene2D& scene) ORTHANC_OVERRIDE;
    
    virtual void Cancel(const Scene2D& scene) ORTHANC_OVERRIDE;
  };
}
