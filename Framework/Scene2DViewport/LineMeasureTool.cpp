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

#include "LineMeasureTool.h"
#include "MeasureToolsToolbox.h"
#include "LayerHolder.h"

#include <Core/Logging.h>

#include <boost/make_shared.hpp>

namespace OrthancStone
{

  LineMeasureTool::LineMeasureTool(
    MessageBroker& broker, ViewportControllerWPtr controllerW)
    : MeasureTool(broker, controllerW)
    , layerHolder_(boost::make_shared<LayerHolder>(controllerW, 1, 5))
  {

  }

  LineMeasureTool::~LineMeasureTool()
  {
    // this measuring tool is a RABI for the corresponding visual layers
    // stored in the 2D scene
    Disable();
    RemoveFromScene();
  }

  void LineMeasureTool::RemoveFromScene()
  {
    if (layerHolder_->AreLayersCreated() && IsSceneAlive())
    {
      layerHolder_->DeleteLayers();
    }
  }
  
  void LineMeasureTool::SetStart(ScenePoint2D start)
  {
    start_ = start;
    RefreshScene();
  }

  void LineMeasureTool::SetEnd(ScenePoint2D end)
  {
    end_ = end;
    RefreshScene();
  }

  void LineMeasureTool::Set(ScenePoint2D start, ScenePoint2D end)
  {
    start_ = start;
    end_ = end;
    RefreshScene();
  }

  void LineMeasureTool::RefreshScene()
  {
    if (IsSceneAlive())
    {
      if (IsEnabled())
      {
        // get the scaling factor 
        const double pixelToScene =
          GetScene()->GetCanvasToSceneTransform().ComputeZoom();

        layerHolder_->CreateLayersIfNeeded();

        {
          // Fill the polyline layer with the measurement line

          PolylineSceneLayer* polylineLayer = layerHolder_->GetPolylineLayer(0);
          polylineLayer->ClearAllChains();
          polylineLayer->SetColor(0, 223, 21);

          {
            PolylineSceneLayer::Chain chain;
            chain.push_back(start_);
            chain.push_back(end_);
            polylineLayer->AddChain(chain, false);
          }

          // handles
          {
            //void AddSquare(PolylineSceneLayer::Chain& chain,const Scene2D& scene,const ScenePoint2D& centerS,const double& sideLength)

            {
              PolylineSceneLayer::Chain chain;
              
              //TODO: take DPI into account
              AddSquare(chain, GetScene(), start_, 10.0 * pixelToScene);
              
              polylineLayer->AddChain(chain, true);
            }

            {
              PolylineSceneLayer::Chain chain;
              
              //TODO: take DPI into account
              AddSquare(chain, GetScene(), end_, 10.0 * pixelToScene);
              
              polylineLayer->AddChain(chain, true);
            }
          }

        }
        {
          // Set the text layer proporeties

          double deltaX = end_.GetX() - start_.GetX();
          double deltaY = end_.GetY() - start_.GetY();
          double squareDist = deltaX * deltaX + deltaY * deltaY;
          double dist = sqrt(squareDist);
          char buf[64];
          sprintf(buf, "%0.02f units", dist);

          // TODO: for now we simply position the text overlay at the middle
          // of the measuring segment
          double midX = 0.5 * (end_.GetX() + start_.GetX());
          double midY = 0.5 * (end_.GetY() + start_.GetY());

          SetTextLayerOutlineProperties(
            GetScene(), layerHolder_, buf, ScenePoint2D(midX, midY));
        }
      }
      else
      {
        RemoveFromScene();
      }
    }
  }
}