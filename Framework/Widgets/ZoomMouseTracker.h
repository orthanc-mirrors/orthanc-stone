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
  class ZoomMouseTracker : public IWorldSceneMouseTracker
  {
  private:
    WorldSceneWidget&  that_;
    double             originalZoom_;
    int                downX_;
    int                downY_;
    double             centerX_;
    double             centerY_;
    bool               idle_;
    double             normalization_;
    
  public:
    ZoomMouseTracker(WorldSceneWidget& that,
                     int x,
                     int y);
    
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
                           double y);
  };
}
