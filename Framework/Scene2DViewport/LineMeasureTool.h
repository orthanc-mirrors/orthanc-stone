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
#include "MeasureTools.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>
#include <cmath>

namespace OrthancStone
{
  class LineMeasureTool : public MeasureTool
  {
  public:
    LineMeasureTool(MessageBroker& broker, ViewportControllerWPtr controllerW)
      : MeasureTool(broker, controllerW)
      , layersCreated(false)
      , polylineZIndex_(-1)
      , textZIndex_(-1)
    {

    }

    ~LineMeasureTool();

    void SetStart(ScenePoint2D start);
    void SetEnd(ScenePoint2D end);
    void Set(ScenePoint2D start, ScenePoint2D end);

  private:
    PolylineSceneLayer* GetPolylineLayer();
    TextSceneLayer*     GetTextLayer();
    virtual void        RefreshScene() ORTHANC_OVERRIDE;
    void                RemoveFromScene();

  private:
    ScenePoint2D start_;
    ScenePoint2D end_;
    bool         layersCreated;
    int          polylineZIndex_;
    int          textZIndex_;
  };

}

