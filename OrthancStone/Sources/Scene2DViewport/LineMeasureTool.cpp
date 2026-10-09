/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "LineMeasureTool.h"

#include "../StoneException.h"
#include "LayerHolder.h"
#include "MeasureCommands.h"
#include "MeasureToolsToolbox.h"

#include <Logging.h>

#include <boost/make_shared.hpp>

namespace OrthancStone
{
  class LineMeasureTool::Tracker : public IFlexiblePointerTracker
  {
  private:
    boost::shared_ptr<LineMeasureTool>         tool_;
    std::unique_ptr<LineMeasureTool::Memento>  originalMemento_;
    ScenePoint2D                               originalClickPosition_;
    LineMeasureTool::LineHighlightArea         modifiedZone_;
    bool                                       alive_;
    boost::weak_ptr<IViewport>&                viewport_;  // TODO Refactoring
    boost::shared_ptr<EditMeasureCommand>      editCommand_;

  public:
    Tracker(const boost::shared_ptr<LineMeasureTool>& tool,
            bool isCreation,
            const Scene2D& scene,
            const PointerEvent& e,
            boost::weak_ptr<IViewport>& viewport) :
      tool_(tool),
      originalClickPosition_(e.GetMainPosition().Apply(scene.GetCanvasToSceneTransform())),
      alive_(true),
      viewport_(viewport)
    {
      if (!tool)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      modifiedZone_ = tool_->LineHitTest(scene, originalClickPosition_);
      originalMemento_.reset(dynamic_cast<LineMeasureTool::Memento*>(tool->CreateMemento()));

      boost::shared_ptr<IViewport> sharedViewport = viewport.lock();
      if (sharedViewport)
      {
        std::unique_ptr<IViewport::ILock> lock(sharedViewport->Lock());
        if (isCreation)
        {
          boost::shared_ptr<MeasureCommand> command(new CreateMeasureCommand(lock->GetController(), tool_));
          lock->GetController().PushCommand(command);
        }
        else
        {
          boost::shared_ptr<EditMeasureCommand> command(new EditMeasureCommand(lock->GetController(), tool_));
          editCommand_ = command;
        }
      }
    }

    bool PointerMove(const PointerEvent &event,
                     Scene2D &scene) ORTHANC_OVERRIDE
    {
      if (alive_)
      {
        const ScenePoint2D scenePos = event.GetMainPosition().Apply(scene.GetCanvasToSceneTransform());
        const ScenePoint2D delta = scenePos - originalClickPosition_;

        switch (modifiedZone_)
        {
          case LineMeasureTool::LineHighlightArea_Start:
            tool_->SetStart(originalMemento_->GetStart() + delta);
            break;

          case LineMeasureTool::LineHighlightArea_End:
            tool_->SetEnd(originalMemento_->GetEnd() + delta);
            break;

          case LineMeasureTool::LineHighlightArea_Segment:
            tool_->SetStart(originalMemento_->GetStart() + delta);
            tool_->SetEnd(originalMemento_->GetEnd() + delta);
            break;

          default:
            LOG(WARNING) << "Warning: please retry the measuring tool editing operation!";
            break;
        }

        return true;
      }
      else
      {
        return false;
      }
    }

    void PointerUp(const PointerEvent &event,
                   Scene2D &scene) ORTHANC_OVERRIDE
    {
      if (alive_ && editCommand_)
      {
        editCommand_->SetMementoModified(tool_->CreateMemento());

        boost::shared_ptr<IViewport> sharedViewport = viewport_.lock();
        if (sharedViewport)
        {
          std::unique_ptr<IViewport::ILock> lock(sharedViewport->Lock());
          lock->GetController().PushCommand(editCommand_);
        }
      }

      alive_ = false;
    }

    void PointerDown(const PointerEvent &event,
                     Scene2D &scene) ORTHANC_OVERRIDE
    {
    }

    bool IsAlive() const ORTHANC_OVERRIDE
    {
      return alive_;
    }

    void Cancel(Scene2D &scene) ORTHANC_OVERRIDE
    {
      alive_ = false;
      tool_->SetMemento(*originalMemento_);
    }

    void SetObserver(IObserver *observer) ORTHANC_OVERRIDE
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  };


  LineMeasureTool::LineMeasureTool(
    boost::weak_ptr<IViewport> viewport):
    MeasureTool(viewport),
#if ORTHANC_STONE_ENABLE_OUTLINED_TEXT == 1
    layerHolder_(boost::shared_ptr<LayerHolder>(new LayerHolder(1, 5))),
#else
    layerHolder_(boost::shared_ptr<LayerHolder>(new LayerHolder(1, 1))),
#endif
    baseLayerIndex_(0),
    lineHighlightArea_(LineHighlightArea_None)
  {

  }

  boost::shared_ptr<LineMeasureTool> LineMeasureTool::Create(boost::weak_ptr<IViewport> viewport)
  {
    boost::shared_ptr<LineMeasureTool> obj(new LineMeasureTool(viewport));
    obj->MeasureTool::PostConstructor();
    obj->RefreshScene();
    return obj;
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
    std::unique_ptr<IViewport::ILock> lock(GetViewportLock());

    if (layerHolder_->AreLayersCreated() && IsSceneAlive())
    {
      layerHolder_->DeleteLayers(lock->GetController().GetScene());
      lock->Invalidate();
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
    ss << "LineMeasureTool. "
       << "Start = (" << start_.GetX() << ", " << start_.GetY() << ") "
       << "End = (" << end_.GetX() << ", " << end_.GetY() << ") ";
    return ss.str();
  }

  void LineMeasureTool::ResetHighlightState()
  {
    SetLineHighlightArea(LineHighlightArea_None);
  }
 
  void LineMeasureTool::Highlight(const Scene2D& scene,
                                  ScenePoint2D p)
  {
    LineHighlightArea lineHighlightArea = LineHitTest(scene, p);
    SetLineHighlightArea(lineHighlightArea);
  }

  LineMeasureTool::LineHighlightArea LineMeasureTool::LineHitTest(const Scene2D& scene,
                                                                  ScenePoint2D p)
  {
    const double pixelToScene = scene.GetCanvasToSceneTransform().ComputeZoom();
    const double SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD =
      pixelToScene * HIT_TEST_MAX_DISTANCE_CANVAS_COORD *
      pixelToScene * HIT_TEST_MAX_DISTANCE_CANVAS_COORD;

    const double sqDistanceFromStart =
      ScenePoint2D::SquaredDistancePtPt(p, start_);

    if (sqDistanceFromStart <= SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD)
      return LineHighlightArea_Start;
    
    const double sqDistanceFromEnd = ScenePoint2D::SquaredDistancePtPt(p, end_);

    if (sqDistanceFromEnd <= SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD)
      return LineHighlightArea_End;

    const double sqDistanceFromPtSegment =
      ScenePoint2D::SquaredDistancePtSegment(start_, end_, p);
    
    if (sqDistanceFromPtSegment <= SQUARED_HIT_TEST_MAX_DISTANCE_SCENE_COORD)
      return LineHighlightArea_Segment;

    return LineHighlightArea_None;
  }

  bool LineMeasureTool::HitTest(const Scene2D& scene,
                                ScenePoint2D p)
  {
    return LineHitTest(scene, p) != LineHighlightArea_None;
  }

  IFlexiblePointerTracker* LineMeasureTool::CreateEditionTracker(bool isCreation,
                                                                 const PointerEvent& e)
  {
    std::unique_ptr<IViewport::ILock> lock(GetViewportLock());

    ViewportController& controller = lock->GetController();
    const Scene2D& scene = controller.GetScene();
    ScenePoint2D scenePos = e.GetMainPosition().Apply(scene.GetCanvasToSceneTransform());

    if (!HitTest(scene, scenePos))
    {
      return NULL;
    }
    else
    {
      return new Tracker(boost::dynamic_pointer_cast<LineMeasureTool>(shared_from_this()), isCreation, scene, e, viewport_);
    }
  }

  MeasureTool::IMemento* LineMeasureTool::CreateMemento() const
  {
    return new Memento(start_, end_);
  }

  void LineMeasureTool::SetMemento(const MeasureTool::IMemento& mementoBase)
  {
    const LineMeasureTool::Memento& memento = dynamic_cast<const LineMeasureTool::Memento&>(mementoBase);
    start_ = memento.GetStart();
    end_ = memento.GetEnd();
    RefreshScene();
  }

  void LineMeasureTool::RefreshScene()
  {
    if (IsSceneAlive())
    {
      if (IsEnabled())
      {
        std::unique_ptr<IViewport::ILock> lock(GetViewportLock());
        ViewportController& controller = lock->GetController();
        Scene2D& scene = controller.GetScene();

        layerHolder_->CreateLayersIfNeeded(scene);

        {
          // Fill the polyline layer with the measurement line

          PolylineSceneLayer* polylineLayer = layerHolder_->GetPolylineLayer(scene, 0);
          if (polylineLayer)
          {
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
                AddSquare(chain, controller.GetScene(), start_, GetHandleSideLengthS(controller.GetScene()));
              
                if (lineHighlightArea_ == LineHighlightArea_Start)
                  polylineLayer->AddChain(chain, true, highlightColor);
                else
                  polylineLayer->AddChain(chain, true, color);
              }

              {
                PolylineSceneLayer::Chain chain;
              
                //TODO: take DPI into account
                AddSquare(chain, controller.GetScene(), end_, GetHandleSideLengthS(controller.GetScene()));
              
                if (lineHighlightArea_ == LineHighlightArea_End)
                  polylineLayer->AddChain(chain, true, highlightColor);
                else
                  polylineLayer->AddChain(chain, true, color);
              }
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

          {

#if ORTHANC_STONE_ENABLE_OUTLINED_TEXT == 1
            SetTextLayerOutlineProperties(
              scene, layerHolder_, buf, ScenePoint2D(midX, midY), 0);
#else
            SetTextLayerProperties(
                                   scene, layerHolder_, buf, ScenePoint2D(midX, midY), 0);
#endif
            lock->Invalidate();
          }
        }
        lock->Invalidate();
      }
      else
      {
        RemoveFromScene();
      }
    }
  }
}
