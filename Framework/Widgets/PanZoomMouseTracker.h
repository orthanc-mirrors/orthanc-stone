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

#include "WorldSceneWidget.h"

namespace OrthancStone
{
  class PanZoomMouseTracker : public IWorldSceneMouseTracker
  {
  private:
    WorldSceneWidget&  that_;
    std::vector<Touch> originalSceneTouches_;
    Touch              originalSceneCenter_;
    Touch              originalDisplayCenter_;
    double             originalPanX_;
    double             originalPanY_;
    double             originalZoom_;
    double             originalDisplayDistanceBetweenTouches_;
    bool               idle_;
    double             normalization_;

  public:
    PanZoomMouseTracker(WorldSceneWidget& that,
                        const std::vector<Touch>& startTouches);
    
    virtual bool HasRender() const
    {
      return false;
    }

    virtual void MouseUp()
    {
    }

    virtual void Render(CairoContext& context,
                        double zoom);

    virtual void MouseMove(int displayX,
                           int displayY,
                           double x,
                           double y,
                           const std::vector<Touch>& displayTouches,
                           const std::vector<Touch>& sceneTouches);
  };
}
