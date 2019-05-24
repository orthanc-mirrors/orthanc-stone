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

  

  bool LineMeasureTool::HitTest(ScenePoint2D p) const
  {
    const double pixelToScene =
      GetScene()->GetCanvasToSceneTransform().ComputeZoom();

    // the hit test will return true if the supplied point (in scene coords)
    // is close to the handle or to the line.

    // since the handle is small, a nice approximation is to defined this
    // as a threshold on the distance between the point and the handle center.

    // this threshold is defined as a constant value in CANVAS units.


    // line equation from two points (non-normalized)
    // (y0-y1)*x + (x1-x0)*xy + (x0*y1 - x1*y0) = 0
    // 
    return false;
  }

  void LineMeasureTool::RefreshScene()
  {
    if (IsSceneAlive())
    {
      if (IsEnabled())
      {
        layerHolder_->CreateLayersIfNeeded();

        {
          // Fill the polyline layer with the measurement line

          PolylineSceneLayer* polylineLayer = layerHolder_->GetPolylineLayer(0);
          polylineLayer->ClearAllChains();
          polylineLayer->SetColor(
            TOOL_LINES_COLOR_RED, 
            TOOL_LINES_COLOR_GREEN, 
            TOOL_LINES_COLOR_BLUE);

          {
            PolylineSceneLayer::Chain chain;
            chain.push_back(start_);
            chain.push_back(end_);
            polylineLayer->AddChain(chain, false);
          }

          // handles
          {
            {
              PolylineSceneLayer::Chain chain;
              
              //TODO: take DPI into account
              AddSquare(chain, GetScene(), start_, 
                GetController()->GetHandleSideLengthS());
              
              polylineLayer->AddChain(chain, true);
            }

            {
              PolylineSceneLayer::Chain chain;
              
              //TODO: take DPI into account
              AddSquare(chain, GetScene(), end_, 
                GetController()->GetHandleSideLengthS());
              
              polylineLayer->AddChain(chain, true);
            }
          }

        }
        {
          // Set the text layer propreties
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
