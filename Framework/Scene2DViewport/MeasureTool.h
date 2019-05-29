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

#include "../Scene2D/PolylineSceneLayer.h"
#include "../Scene2D/Scene2D.h"
#include "../Scene2D/ScenePoint2D.h"
#include "../Scene2D/TextSceneLayer.h"
#include "../Scene2DViewport/PredeclaredTypes.h"
#include "../Scene2DViewport/ViewportController.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>
#include <cmath>

namespace OrthancStone
{
  class MeasureTool : public IObserver
  {
  public:
    virtual ~MeasureTool();

    /**
    Enabled tools are rendered in the scene.
    */
    void Enable();

    /**
    Disabled tools are not rendered in the scene. This is useful to be able
    to use them as their own memento in command stacks (when a measure tool
    creation command has been undone, the measure remains alive in the
    command object but is disabled so that it can be redone later on easily)
    */
    void Disable();

    /**
    This method is called when the scene transform changes. It allows to 
    recompute the visual elements whose content depend upon the scene transform
    */
    void OnSceneTransformChanged(
      const ViewportController::SceneTransformChanged& message);
    
    /**
    This function must be implemented by the measuring tool to return whether
    a given point in scene coords is close to the measuring tool.

    This is used for mouse hover highlighting.

    It is assumed that if the pointer position leads to this function returning
    true, then a click at that position will return a tracker to edit the 
    measuring tool
    */
    virtual bool HitTest(ScenePoint2D p) const = 0;
  protected:
    MeasureTool(MessageBroker& broker, boost::weak_ptr<ViewportController> controllerW);

    /**
    The measuring tool may exist in a standalone fashion, without any available
    scene (because the controller is dead or dying). This call allows to check 
    before accessing the scene.
    */
    bool IsSceneAlive() const;
    
    /**
    This is the meat of the tool: this method must [create (if needed) and]
    update the layers and their data according to the measure tool kind and
    current state. This is repeatedly called during user interaction
    */
    virtual void RefreshScene() = 0;

    boost::shared_ptr<const ViewportController> GetController() const;
    boost::shared_ptr<ViewportController>      GetController();

    boost::shared_ptr<const Scene2D>            GetScene() const;
    boost::shared_ptr<Scene2D>                 GetScene();

    /**
    enabled_ is not accessible by subclasses because there is a state machine
    that we do not wanna mess with
    */
    bool IsEnabled() const;

  private:
    boost::weak_ptr<ViewportController> controllerW_;
    bool     enabled_;
  };
}

extern void TrackerSample_SetInfoDisplayMessage(
  std::string key, std::string value);
