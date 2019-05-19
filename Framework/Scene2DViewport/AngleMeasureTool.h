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

#include "MeasureTools.h"

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
  class AngleMeasureTool : public MeasureTool
  {
  public:
    AngleMeasureTool(MessageBroker& broker, ViewportControllerWPtr controllerW)
      : MeasureTool(broker, controllerW)
      , layersCreated(false)
      , polylineZIndex_(-1)
      , textBaseZIndex_(-1)
    {

    }

    ~AngleMeasureTool();

    void SetSide1End(ScenePoint2D start);
    void SetCenter(ScenePoint2D start);
    void SetSide2End(ScenePoint2D start);

  private:
    PolylineSceneLayer* GetPolylineLayer();
    
    // 0 --> 3 are for the text background (outline)
    // 4 is for the actual text
    TextSceneLayer*     GetTextLayer(int index);
    virtual void        RefreshScene() ORTHANC_OVERRIDE;
    void                RemoveFromScene();

  private:
    ScenePoint2D side1End_;
    ScenePoint2D side2End_;
    ScenePoint2D center_;
    bool         layersCreated;
    int          polylineZIndex_;
    int          textBaseZIndex_;
  };

}


