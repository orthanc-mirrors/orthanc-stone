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

#include "AngleMeasureTool.h"
#include "MeasureToolsToolbox.h"
#include "LayerHolder.h"

#include <Core/Logging.h>

#include <boost/math/constants/constants.hpp>
#include <boost/make_shared.hpp>

// <HACK>
// REMOVE THIS
#ifndef NDEBUG
extern void 
TrackerSample_SetInfoDisplayMessage(std::string key, std::string value);
#endif
// </HACK>

namespace OrthancStone
{
  // the params in the LayerHolder ctor specify the number of polyline and text
  // layers
  AngleMeasureTool::AngleMeasureTool(
    MessageBroker& broker, ViewportControllerWPtr controllerW)
    : MeasureTool(broker, controllerW)
    , layerHolder_(boost::make_shared<LayerHolder>(controllerW,1,5))
  {

  }

  AngleMeasureTool::~AngleMeasureTool()
  {
    // this measuring tool is a RABI for the corresponding visual layers
    // stored in the 2D scene
    Disable();
    RemoveFromScene();
  }

  void AngleMeasureTool::RemoveFromScene()
  {
    if (layerHolder_->AreLayersCreated() && IsSceneAlive())
    {
      layerHolder_->DeleteLayers();
    }
  }

  void AngleMeasureTool::SetSide1End(ScenePoint2D pt)
  {
    side1End_ = pt;
    RefreshScene();
  }

  void AngleMeasureTool::SetSide2End(ScenePoint2D pt)
  {
    side2End_ = pt;
    RefreshScene();
  }

  void AngleMeasureTool::SetCenter(ScenePoint2D pt)
  {
    center_ = pt;
    RefreshScene();
  }
  
  void AngleMeasureTool::RefreshScene()
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
          // Fill the polyline layer with the measurement lines
          PolylineSceneLayer* polylineLayer = layerHolder_->GetPolylineLayer(0);
          polylineLayer->ClearAllChains();
          polylineLayer->SetColor(0, 183, 17);


          // sides
          {
            {
              PolylineSceneLayer::Chain chain;
              chain.push_back(side1End_);
              chain.push_back(center_);
              polylineLayer->AddChain(chain, false);
            }
            {
              PolylineSceneLayer::Chain chain;
              chain.push_back(side2End_);
              chain.push_back(center_);
              polylineLayer->AddChain(chain, false);
            }
          }

          // Create the handles
          {
            {
              PolylineSceneLayer::Chain chain;
              //TODO: take DPI into account
              AddSquare(chain, GetScene(), side1End_, 10.0 * pixelToScene);
              polylineLayer->AddChain(chain, true);
            }
            {
              PolylineSceneLayer::Chain chain;
              //TODO: take DPI into account
              AddSquare(chain, GetScene(), side2End_, 10.0 * pixelToScene); 
              polylineLayer->AddChain(chain, true);
            }
          }

          // Create the arc
          {
            PolylineSceneLayer::Chain chain;

            const double ARC_RADIUS_CANVAS_COORD = 30.0;
            AddShortestArc(chain, side1End_, center_, side2End_,
                           ARC_RADIUS_CANVAS_COORD * pixelToScene);
            polylineLayer->AddChain(chain, false);
          }
        }
        {
          // Set the text layer

          double p1cAngle = atan2(
            side1End_.GetY() - center_.GetY(),
            side1End_.GetX() - center_.GetX());

          double p2cAngle = atan2(
            side2End_.GetY() - center_.GetY(),
            side2End_.GetX() - center_.GetX());

          double delta = NormalizeAngle(p2cAngle - p1cAngle);
          double theta = p1cAngle + delta / 2;

          const double TEXT_CENTER_DISTANCE_CANVAS_COORD = 90;

          double offsetX = TEXT_CENTER_DISTANCE_CANVAS_COORD * cos(theta);
          double offsetY = TEXT_CENTER_DISTANCE_CANVAS_COORD * sin(theta);

          double pointX = center_.GetX() + offsetX * pixelToScene;
          double pointY = center_.GetY() + offsetY * pixelToScene;

          char buf[64];
          double angleDeg = RadiansToDegrees(delta);

          // http://www.ltg.ed.ac.uk/~richard/utf-8.cgi?input=00B0&mode=hex
          sprintf(buf, "%0.02f\xc2\xb0", angleDeg);

          SetTextLayerOutlineProperties(
            GetScene(), layerHolder_, buf, ScenePoint2D(pointX, pointY));

          // TODO:make it togglable
          bool enableInfoDisplay = false;
          if (enableInfoDisplay)
          {
            TrackerSample_SetInfoDisplayMessage("center_.GetX()",
              boost::lexical_cast<std::string>(center_.GetX()));

            TrackerSample_SetInfoDisplayMessage("center_.GetY()",
              boost::lexical_cast<std::string>(center_.GetY()));

            TrackerSample_SetInfoDisplayMessage("side1End_.GetX()",
              boost::lexical_cast<std::string>(side1End_.GetX()));

            TrackerSample_SetInfoDisplayMessage("side1End_.GetY()",
              boost::lexical_cast<std::string>(side1End_.GetY()));

            TrackerSample_SetInfoDisplayMessage("side2End_.GetX()",
              boost::lexical_cast<std::string>(side2End_.GetX()));

            TrackerSample_SetInfoDisplayMessage("side2End_.GetY()",
              boost::lexical_cast<std::string>(side2End_.GetY()));

            TrackerSample_SetInfoDisplayMessage("p1cAngle (deg)",
              boost::lexical_cast<std::string>(RadiansToDegrees(p1cAngle)));

            TrackerSample_SetInfoDisplayMessage("delta (deg)",
              boost::lexical_cast<std::string>(RadiansToDegrees(delta)));

            TrackerSample_SetInfoDisplayMessage("theta (deg)",
              boost::lexical_cast<std::string>(RadiansToDegrees(theta)));

            TrackerSample_SetInfoDisplayMessage("p2cAngle (deg)",
              boost::lexical_cast<std::string>(RadiansToDegrees(p2cAngle)));

            TrackerSample_SetInfoDisplayMessage("offsetX (pix)",
              boost::lexical_cast<std::string>(offsetX));

            TrackerSample_SetInfoDisplayMessage("offsetY (pix)",
              boost::lexical_cast<std::string>(offsetY));

            TrackerSample_SetInfoDisplayMessage("pointX",
              boost::lexical_cast<std::string>(pointX));

            TrackerSample_SetInfoDisplayMessage("pointY",
              boost::lexical_cast<std::string>(pointY));

            TrackerSample_SetInfoDisplayMessage("angleDeg",
              boost::lexical_cast<std::string>(angleDeg));
          }
        }
      }
      else
      {
        RemoveFromScene();
      }
    }
  }
}
