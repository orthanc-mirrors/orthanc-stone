/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include "../Viewport/CairoContext.h"

namespace OrthancStone
{

  // this is tracking a mouse in scene coordinates/mm unlike
  // the IMouseTracker that is tracking a mouse
  // in screen coordinates/pixels.
  class IWorldSceneMouseTracker : public boost::noncopyable
  {
  public:
    virtual ~IWorldSceneMouseTracker()
    {
    }

    virtual bool HasRender() const = 0;

    virtual void Render(CairoContext& context,
                        double zoom) = 0;

    virtual void MouseUp() = 0;

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY) = 0;
  };
}
