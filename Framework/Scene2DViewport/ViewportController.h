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

#include "PointerTypes.h"

#include <Framework/Scene2D/Scene2D.h>
#include <Framework/Scene2D/PointerEvent.h>
#include <Framework/Scene2DViewport/MeasureTools.h>
#include <Framework/Scene2DViewport/IFlexiblePointerTracker.h>

namespace OrthancStone
{
  /**
  This object is responsible for hosting a scene, responding to messages from
  the model and updating the scene accordingly.

  It contains the list of active measuring tools as well as the stack
  where measuring tool commands are stored.

  The active tracker is also stored in the viewport controller.

  Each canvas or other GUI area where we want to display a 2D image, either 
  directly or through slicing must be assigned a ViewportController.
  */
  class ViewportController : public boost::noncopyable
  {
  public:
    ViewportController(MessageBroker& broker);

    Scene2DPtr GetScene();

    /** 
    This method is called by the GUI system and should update/delete the
    current tracker
    */
    bool HandlePointerEvent(PointerEvent e);

    /**
    This method returns the list of measure tools containing the supplied point
    (in scene coords). A tracker can then be requested from the chosen 
    measure tool, if needed
    */
    std::vector<MeasureToolPtr> HitTestMeasureTools(ScenePoint2D p);

    /**
    With this method, the object takes ownership of the supplied tracker and
    updates it according to user interaction
    */
    void SetActiveTracker(FlexiblePointerTrackerPtr tracker);

  private:
    MessageBroker&            broker_;
    Scene2DWPtr               scene_;
    FlexiblePointerTrackerPtr tracker_;
  };
}
