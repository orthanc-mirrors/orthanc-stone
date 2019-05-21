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

#include <Framework/Scene2DViewport/PointerTypes.h>
#include <Framework/Scene2DViewport/ViewportController.h>

#include <Framework/Scene2D/Scene2D.h>
#include <Framework/Scene2D/ScenePoint2D.h>
#include <Framework/Scene2D/PolylineSceneLayer.h>
#include <Framework/Scene2D/TextSceneLayer.h>

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

  protected:
    MeasureTool(MessageBroker& broker, ViewportControllerWPtr controllerW);

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

    ViewportControllerPtr GetController();
    Scene2DPtr GetScene();
    
    /**
    enabled_ is not accessible by subclasses because there is a state machine
    that we do not wanna mess with
    */
    bool IsEnabled() const;

  private:
    ViewportControllerWPtr controllerW_;
    bool     enabled_;
  };
}


extern void TrackerSample_SetInfoDisplayMessage(
  std::string key, std::string value);
