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

#include "IViewportInteractor.h"

namespace OrthancStone
{
  class DefaultViewportInteractor : public IViewportInteractor
  {
  private:
    // Index of the layer whose windowing is altered by clicking the
    // left mouse button
    int  windowingLayer_;
    
  public:
    DefaultViewportInteractor() :
      windowingLayer_(0)
    {
    }

    int GetWindowingLayer() const
    {
      return windowingLayer_;
    }

    void SetWindowingLayer(int layerIndex)
    {
      windowingLayer_ = layerIndex;
    }
    
    virtual IFlexiblePointerTracker* CreateTracker(boost::shared_ptr<IViewport> viewport,
                                                   const PointerEvent& event,
                                                   unsigned int viewportWidth,
                                                   unsigned int viewportHeight) ORTHANC_OVERRIDE;
  };
}
