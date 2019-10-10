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
#include "EditLineMeasureTracker.h"
#include "LayerHolder.h"
#include "../StoneException.h"

#include <Core/Logging.h>

#include <boost/make_shared.hpp>

namespace OrthancStone
{

  LineMeasureTool::LineMeasureTool(
    MessageBroker& broker, boost::weak_ptr<ViewportController> controllerW)
    : MeasureTool(broker, controllerW)
#if ORTHANC_STONE_ENABLE_OUTLINED_TEXT == 1
    , layerHolder_(boost::make_shared<LayerHolder>(controllerW, 1, 5))
#else
    , layerHolder_(boost::make_shared<LayerHolder>(controllerW, 1, 1))
#endif
    , lineHighlightArea_(LineHighlightArea_None)
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

  void LineMeasureTool::SetLineHighlightArea(LineHighlightArea area)
  {
    if (lineHighlightArea_ != area)
    {
      lineHighlightArea_ = area;
      RefreshScene();
    }
  }

  std::string LineMeasureTool::GetDescription()
  {
    std::stringstream ss;
    ss << "LineMeasureTool. Start = " << start_ << " End = " << end_;
    return ss.str();
  }

  void LineMeasureTool::ResetHighlightState()
  {
    SetLineHighlightArea(LineHighlightArea_None);
  }
 
  void LineMeasureTool::Highlight(ScenePoint2D p)
  {
    LineHighlightArea lineHighlightArea = LineHitTest(p);
    SetLineHighlightArea(lineHighlightArea);
  }

  LineMeasureTool::LineHighlightArea LineMeasureTool::LineHitTest(ScenePoint2D p) const
  {
    const double pixelToScene =
      GetController()->GetScene().GetCanvasToSceneTransform().ComputeZoom();
    const double SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD = pixelToScene * HIT_TEST_MAX_DISTANCE_CANVAS_COORD * pixelToScene * HIT_TEST_MAX_DISTANCE_CANVAS_COORD;

    const double sqDistanceFromStart = ScenePoint2D::SquaredDistancePtPt(p, start_);
    if (sqDistanceFromStart <= SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD)
      return LineHighlightArea_Start;
    
    const double sqDistanceFromEnd = ScenePoint2D::SquaredDistancePtPt(p, end_);
    if (sqDistanceFromEnd <= SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD)
      return LineHighlightArea_End;

    const double sqDistanceFromPtSegment = ScenePoint2D::SquaredDistancePtSegment(start_, end_, p);
    if (sqDistanceFromPtSegment <= SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD)
      return LineHighlightArea_Segment;

    return LineHighlightArea_None;
  }

  bool LineMeasureTool::HitTest(ScenePoint2D p) const
  {
    return LineHitTest(p) != LineHighlightArea_None;
  }

  boost::shared_ptr<IFlexiblePointerTracker> LineMeasureTool::CreateEditionTracker(const PointerEvent& e)
  {
    ScenePoint2D scenePos = e.GetMainPosition().Apply(
      GetController()->GetScene().GetCanvasToSceneTransform());

    if (!HitTest(scenePos))
      return boost::shared_ptr<IFlexiblePointerTracker>();

    /**
      new EditLineMeasureTracker(
        boost::shared_ptr<LineMeasureTool> measureTool;
        MessageBroker & broker,
        boost::weak_ptr<ViewportController>          controllerW,
        const PointerEvent & e);
    */
    boost::shared_ptr<EditLineMeasureTracker> editLineMeasureTracker(
      new EditLineMeasureTracker(shared_from_this(), GetBroker(), GetController(), e));
    return editLineMeasureTracker;
  }


  boost::shared_ptr<MeasureToolMemento> LineMeasureTool::GetMemento() const
  {
    boost::shared_ptr<LineMeasureToolMemento> memento(new LineMeasureToolMemento());
    memento->start_ = start_;
    memento->end_ = end_;
    return memento;
  }

  void LineMeasureTool::SetMemento(boost::shared_ptr<MeasureToolMemento> mementoBase)
  {
    boost::shared_ptr<LineMeasureToolMemento> memento = boost::dynamic_pointer_cast<LineMeasureToolMemento>(mementoBase);
    ORTHANC_ASSERT(memento.get() != NULL, "Internal error: wrong (or bad) memento");
    start_ = memento->start_;
    end_ = memento->end_;
    RefreshScene();
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

          const Color color(TOOL_LINES_COLOR_RED, 
                            TOOL_LINES_COLOR_GREEN, 
                            TOOL_LINES_COLOR_BLUE);

          const Color highlightColor(TOOL_LINES_HL_COLOR_RED,
                                     TOOL_LINES_HL_COLOR_GREEN,
                                     TOOL_LINES_HL_COLOR_BLUE);

          {
            PolylineSceneLayer::Chain chain;
            chain.push_back(start_);
            chain.push_back(end_);
            if(lineHighlightArea_ == LineHighlightArea_Segment)
              polylineLayer->AddChain(chain, false, highlightColor);
            else
              polylineLayer->AddChain(chain, false, color);
          }

          // handles
          {
            {
              PolylineSceneLayer::Chain chain;
              
              //TODO: take DPI into account
              AddSquare(chain, GetController()->GetScene(), start_, 
                GetController()->GetHandleSideLengthS());
              
              if (lineHighlightArea_ == LineHighlightArea_Start)
                polylineLayer->AddChain(chain, true, highlightColor);
              else
                polylineLayer->AddChain(chain, true, color);
            }

            {
              PolylineSceneLayer::Chain chain;
              
              //TODO: take DPI into account
              AddSquare(chain, GetController()->GetScene(), end_, 
                GetController()->GetHandleSideLengthS());
              
              if (lineHighlightArea_ == LineHighlightArea_End)
                polylineLayer->AddChain(chain, true, highlightColor);
              else
                polylineLayer->AddChain(chain, true, color);
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
          sprintf(buf, "%0.02f mm", dist);

          // TODO: for now we simply position the text overlay at the middle
          // of the measuring segment
          double midX = 0.5 * (end_.GetX() + start_.GetX());
          double midY = 0.5 * (end_.GetY() + start_.GetY());

#if ORTHANC_STONE_ENABLE_OUTLINED_TEXT == 1
          SetTextLayerOutlineProperties(
            GetController()->GetScene(), layerHolder_, buf, ScenePoint2D(midX, midY), 0);
#else
          SetTextLayerProperties(
            GetController()->GetScene(), layerHolder_, buf, ScenePoint2D(midX, midY), 0);
#endif
        }
      }
      else
      {
        RemoveFromScene();
      }
    }
  }
}
