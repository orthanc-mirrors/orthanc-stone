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

#include <Core/Logging.h>

#include <boost/math/constants/constants.hpp>

namespace OrthancStone
{
  AngleMeasureTool::~AngleMeasureTool()
  {
    // this measuring tool is a RABI for the corresponding visual layers
    // stored in the 2D scene
    Disable();
    RemoveFromScene();
  }

  void AngleMeasureTool::RemoveFromScene()
  {
    if (layersCreated)
    {
      assert(GetScene().HasLayer(polylineZIndex_));
      assert(GetScene().HasLayer(textZIndex_));
      GetScene().DeleteLayer(polylineZIndex_);
      GetScene().DeleteLayer(textZIndex_);
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
  
  PolylineSceneLayer* AngleMeasureTool::GetPolylineLayer()
  {
    assert(GetScene().HasLayer(polylineZIndex_));
    ISceneLayer* layer = &(GetScene().GetLayer(polylineZIndex_));
    PolylineSceneLayer* concreteLayer = dynamic_cast<PolylineSceneLayer*>(layer);
    assert(concreteLayer != NULL);
    return concreteLayer;
  }

  TextSceneLayer* AngleMeasureTool::GetTextLayer()
  {
    assert(GetScene().HasLayer(textZIndex_));
    ISceneLayer* layer = &(GetScene().GetLayer(textZIndex_));
    TextSceneLayer* concreteLayer = dynamic_cast<TextSceneLayer*>(layer);
    assert(concreteLayer != NULL);
    return concreteLayer;
  }


  void AngleMeasureTool::RefreshScene()
  {
    if (IsEnabled())
    {
      // get the scaling factor 
      const double pixelToScene =
        GetScene().GetCanvasToSceneTransform().ComputeZoom();

      if (!layersCreated)
      {
        // Create the layers if need be

        assert(textZIndex_ == -1);
        {
          polylineZIndex_ = GetScene().GetMaxDepth() + 100;
          //LOG(INFO) << "set polylineZIndex_ to: " << polylineZIndex_;
          std::auto_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer());
          GetScene().SetLayer(polylineZIndex_, layer.release());
        }
        {
          textZIndex_ = GetScene().GetMaxDepth() + 100;
          //LOG(INFO) << "set textZIndex_ to: " << textZIndex_;
          std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer());
          GetScene().SetLayer(textZIndex_, layer.release());
        }
        layersCreated = true;
      }
      else
      {
        assert(GetScene().HasLayer(polylineZIndex_));
        assert(GetScene().HasLayer(textZIndex_));
      }
      {
        // Fill the polyline layer with the measurement line

        PolylineSceneLayer* polylineLayer = GetPolylineLayer();
        polylineLayer->ClearAllChains();
        polylineLayer->SetColor(0, 223, 21);

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

        // handles
        {
          //void AddSquare(PolylineSceneLayer::Chain& chain,const Scene2D& scene,const ScenePoint2D& centerS,const double& sideLength)

          {
            PolylineSceneLayer::Chain chain;
            AddSquare(chain, GetScene(), side1End_, 10.0* pixelToScene); //TODO: take DPI into account
            polylineLayer->AddChain(chain, true);
          }

          {
            PolylineSceneLayer::Chain chain;
            AddSquare(chain, GetScene(), side2End_, 10.0* pixelToScene); //TODO: take DPI into account
            polylineLayer->AddChain(chain, true);
          }
        }

        // arc
        {
          PolylineSceneLayer::Chain chain;

          const double ARC_RADIUS_CANVAS_COORD = 20.0;
          AddShortestArc(chain, GetScene(), side1End_, center_, side2End_, 
            ARC_RADIUS_CANVAS_COORD*pixelToScene);
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
        double theta = delta/2;

        const double TEXT_CENTER_DISTANCE_CANVAS_COORD = 40.0;

        double offsetX = TEXT_CENTER_DISTANCE_CANVAS_COORD * cos(theta);
        double offsetY = TEXT_CENTER_DISTANCE_CANVAS_COORD * sin(theta);
        double pointX = center_.GetX() + offsetX;
        double pointY = center_.GetY() + offsetY;

        TextSceneLayer* textLayer = GetTextLayer();

        char buf[64];
        double angleDeg = RadiansToDegrees(delta);
        sprintf(buf, "%0.02f deg", angleDeg);
        textLayer->SetText(buf);
        textLayer->SetColor(0, 223, 21);

        ScenePoint2D textAnchor;
        //GetPositionOnBisectingLine(
        //  textAnchor, side1End_, center_, side2End_, 40.0*pixelToScene);
        textLayer->SetPosition(pointX, pointY);
      }
    }
    else
    {
      if (layersCreated)
      {
        RemoveFromScene();
        layersCreated = false;
      }
    }
  }


}
