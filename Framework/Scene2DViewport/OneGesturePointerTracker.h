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

#include "IFlexiblePointerTracker.h"

namespace OrthancStone
{
  /**
  This base is class allows to write simple trackers that deal with single 
  drag gestures with only one touch. It is *not* suitable for multi-touch and
  multi-state trackers where various mouse operations need to be handled.

  In order to write such a tracker:
  - subclass this class
  - you may store the initial click/touch position in the constructor
  - implement PointerMove to react to pointer/touch events
  - implement Cancel to restore the state at initial tracker creation time

  */
  class OneGesturePointerTracker : public IFlexiblePointerTracker
  {
  public:
    OneGesturePointerTracker(boost::weak_ptr<ViewportController> controllerW);
    virtual void PointerUp(const PointerEvent& event) ORTHANC_OVERRIDE;
    virtual void PointerDown(const PointerEvent& event) ORTHANC_OVERRIDE;
    virtual bool IsAlive() const ORTHANC_OVERRIDE;
  
  protected:
    boost::shared_ptr<ViewportController>  GetController();

  private:
    boost::weak_ptr<ViewportController> controllerW_;
    bool                   alive_;
    int                    currentTouchCount_;
  };
}

