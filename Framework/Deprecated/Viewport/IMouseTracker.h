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

#include "../../Wrappers/CairoSurface.h"
#include <vector>

namespace Deprecated
{
  struct Touch
  {
    float x;
    float y;

    Touch(float x, float y)
    : x(x),
      y(y)
    {
    }
    Touch()
      : x(0.0f),
        y(0.0f)
    {
    }
  };


  // this is tracking a mouse in screen coordinates/pixels unlike
  // the IWorldSceneMouseTracker that is tracking a mouse
  // in scene coordinates/mm.
  class IMouseTracker : public boost::noncopyable
  {
  public:
    virtual ~IMouseTracker()
    {
    }
    
    virtual void Render(Orthanc::ImageAccessor& surface) = 0;

    virtual void MouseUp() = 0;

    // Returns "true" iff. the background scene must be repainted
    virtual void MouseMove(int x, 
                           int y,
                           const std::vector<Touch>& displayTouches) = 0;
  };
}
