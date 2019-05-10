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

#include <Core/Logging.h>
#include "MeasureTools.h"
#include <boost/math/constants/constants.hpp>

namespace OrthancStone
{
  void MeasureTool::Enable()
  {
    enabled_ = true;
    RefreshScene();
  }

  void MeasureTool::Disable()
  {
    enabled_ = false;
    RefreshScene();
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
    if (layersCreated)
    {
      assert(GetScene().HasLayer(polylineZIndex_));
      assert(GetScene().HasLayer(textZIndex_));
      GetScene().DeleteLayer(polylineZIndex_);
      GetScene().DeleteLayer(textZIndex_);
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

  PolylineSceneLayer* LineMeasureTool::GetPolylineLayer()
  {
    assert(GetScene().HasLayer(polylineZIndex_));
    ISceneLayer* layer = &(GetScene().GetLayer(polylineZIndex_));
    PolylineSceneLayer* concreteLayer = dynamic_cast<PolylineSceneLayer*>(layer);
    assert(concreteLayer != NULL);
    return concreteLayer;
  }

  TextSceneLayer* LineMeasureTool::GetTextLayer()
  {
    assert(GetScene().HasLayer(textZIndex_));
    ISceneLayer* layer = &(GetScene().GetLayer(textZIndex_));
    TextSceneLayer* concreteLayer = dynamic_cast<TextSceneLayer*>(layer);
    assert(concreteLayer != NULL);
    return concreteLayer;
  }

  namespace
  {
    /**
    This function will create a square around the center point supplied in
    scene coordinates, with a side length given in canvas coordinates. The
    square sides are parallel to the canvas boundaries.
    */
    void AddSquare(PolylineSceneLayer::Chain& chain,
      const Scene2D&      scene,
      const ScenePoint2D& centerS, 
      const double&       sideLength)
    {
      chain.clear();
      chain.reserve(4);
      ScenePoint2D centerC = centerS.Apply(scene.GetSceneToCanvasTransform());
      //TODO: take DPI into account
      double handleLX = centerC.GetX() - sideLength / 2;
      double handleTY = centerC.GetY() - sideLength / 2;
      double handleRX = centerC.GetX() + sideLength / 2;
      double handleBY = centerC.GetY() + sideLength / 2;
      ScenePoint2D LTC(handleLX, handleTY);
      ScenePoint2D RTC(handleRX, handleTY);
      ScenePoint2D RBC(handleRX, handleBY);
      ScenePoint2D LBC(handleLX, handleBY);

      ScenePoint2D startLT = LTC.Apply(scene.GetCanvasToSceneTransform());
      ScenePoint2D startRT = RTC.Apply(scene.GetCanvasToSceneTransform());
      ScenePoint2D startRB = RBC.Apply(scene.GetCanvasToSceneTransform());
      ScenePoint2D startLB = LBC.Apply(scene.GetCanvasToSceneTransform());

      chain.push_back(startLT);
      chain.push_back(startRT);
      chain.push_back(startRB);
      chain.push_back(startLB);
    }

    void AddCircle(PolylineSceneLayer::Chain& chain,
      const Scene2D&      scene,
      const ScenePoint2D& centerS,
      const double&       radiusS)
    {
      chain.clear();
      chain.reserve(4);
      //ScenePoint2D centerC = centerS.Apply(scene.GetSceneToCanvasTransform());
      //TODO: take DPI into account
      
      // TODO: automatically compute the number for segments for smooth 
      // display based on the radius in pixels.
      int lineCount = 50;
      
      double angleIncr = (2.0 * boost::math::constants::pi<double>())
        / static_cast<double>(lineCount);

      double theta = 0;
      for (int i = 0; i < lineCount; ++i)
      {
        double offsetX = radiusS * cos(theta);
        double offsetY = radiusS * sin(theta);
        double pointX = centerS.GetX() + offsetX;
        double pointY = centerS.GetY() + offsetY;
        chain.push_back(ScenePoint2D(pointX, pointY));
        theta += angleIncr;
      }
    }

#if 0
    void AddEllipse(PolylineSceneLayer::Chain& chain,
      const Scene2D&      scene,
      const ScenePoint2D& centerS,
      const double&       halfHAxis,
      const double&       halfVAxis)
    {
      chain.clear();
      chain.reserve(4);
      ScenePoint2D centerC = centerS.Apply(scene.GetSceneToCanvasTransform());
      //TODO: take DPI into account
      double handleLX = centerC.GetX() - sideLength / 2;
      double handleTY = centerC.GetY() - sideLength / 2;
      double handleRX = centerC.GetX() + sideLength / 2;
      double handleBY = centerC.GetY() + sideLength / 2;
      ScenePoint2D LTC(handleLX, handleTY);
      ScenePoint2D RTC(handleRX, handleTY);
      ScenePoint2D RBC(handleRX, handleBY);
      ScenePoint2D LBC(handleLX, handleBY);

      ScenePoint2D startLT = LTC.Apply(scene.GetCanvasToSceneTransform());
      ScenePoint2D startRT = RTC.Apply(scene.GetCanvasToSceneTransform());
      ScenePoint2D startRB = RBC.Apply(scene.GetCanvasToSceneTransform());
      ScenePoint2D startLB = LBC.Apply(scene.GetCanvasToSceneTransform());

      chain.push_back(startLT);
      chain.push_back(startRT);
      chain.push_back(startRB);
      chain.push_back(startLB);
    }
#endif


  }


  void LineMeasureTool::RefreshScene()
  {
    if (IsEnabled())
    {
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
            AddSquare(chain, GetScene(), start_, 10.0); //TODO: take DPI into account
            polylineLayer->AddChain(chain, true);
          }

          {
            PolylineSceneLayer::Chain chain;
            AddSquare(chain, GetScene(), end_, 10.0); //TODO: take DPI into account
            polylineLayer->AddChain(chain, true);
          }

          //ScenePoint2D startC = start_.Apply(GetScene().GetSceneToCanvasTransform());
          //double squareSize = 10.0; 
          //double startHandleLX = startC.GetX() - squareSize/2;
          //double startHandleTY = startC.GetY() - squareSize / 2;
          //double startHandleRX = startC.GetX() + squareSize / 2;
          //double startHandleBY = startC.GetY() + squareSize / 2;
          //ScenePoint2D startLTC(startHandleLX, startHandleTY);
          //ScenePoint2D startRTC(startHandleRX, startHandleTY);
          //ScenePoint2D startRBC(startHandleRX, startHandleBY);
          //ScenePoint2D startLBC(startHandleLX, startHandleBY);

          //ScenePoint2D startLT = startLTC.Apply(GetScene().GetCanvasToSceneTransform());
          //ScenePoint2D startRT = startRTC.Apply(GetScene().GetCanvasToSceneTransform());
          //ScenePoint2D startRB = startRBC.Apply(GetScene().GetCanvasToSceneTransform());
          //ScenePoint2D startLB = startLBC.Apply(GetScene().GetCanvasToSceneTransform());

          //PolylineSceneLayer::Chain chain;
          //chain.push_back(startLT);
          //chain.push_back(startRT);
          //chain.push_back(startRB);
          //chain.push_back(startLB);
          //polylineLayer->AddChain(chain, true);
        }
       
      }
      {
        // Set the text layer proporeties

        TextSceneLayer* textLayer = GetTextLayer();
        double deltaX = end_.GetX() - start_.GetX();
        double deltaY = end_.GetY() - start_.GetY();
        double squareDist = deltaX * deltaX + deltaY * deltaY;
        double dist = sqrt(squareDist);
        char buf[64];
        sprintf(buf, "%0.02f units", dist);
        textLayer->SetText(buf);
        textLayer->SetColor(0, 223, 21);

        // TODO: for now we simply position the text overlay at the middle
        // of the measuring segment
        double midX = 0.5*(end_.GetX() + start_.GetX());
        double midY = 0.5*(end_.GetY() + start_.GetY());
        textLayer->SetPosition(midX, midY);
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

