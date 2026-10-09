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

#pragma once

#include "../Scene2DViewport/LayerHolder.h"
#include "../Scene2D/ScenePoint2D.h"
#include "../Scene2D/PolylineSceneLayer.h"
#include "../Scene2D/TextSceneLayer.h"
#include "MeasureTool.h"

#include <boost/enable_shared_from_this.hpp>

#include <vector>
#include <cmath>

namespace OrthancStone
{
  class AngleMeasureTool : public MeasureTool
  {
  public:
    class Memento : public IMemento
    {
    private:
      ScenePoint2D   center_;
      ScenePoint2D   side1End_;
      ScenePoint2D   side2End_;

    public:
      Memento(const ScenePoint2D& center,
              const ScenePoint2D& side1End,
              const ScenePoint2D& side2End) :
        center_(center),
        side1End_(side1End),
        side2End_(side2End)
      {
      }

      const ScenePoint2D& GetCenter() const
      {
        return center_;
      }

      const ScenePoint2D& GetSide1End() const
      {
        return side1End_;
      }

      const ScenePoint2D& GetSide2End() const
      {
        return side2End_;
      }
    };

  public:
    static boost::shared_ptr<AngleMeasureTool> Create(boost::weak_ptr<IViewport> viewport,
                                                      const boost::shared_ptr<ViewportController>& controller);

    ~AngleMeasureTool();

    void SetSide1End(ScenePoint2D start);
    void SetCenter(ScenePoint2D start);
    void SetSide2End(ScenePoint2D start);

    virtual bool HitTest(const Scene2D& scene,
                         ScenePoint2D p) ORTHANC_OVERRIDE;
    virtual void Highlight(const Scene2D& scene,
                           ScenePoint2D p) ORTHANC_OVERRIDE;
    virtual void ResetHighlightState() ORTHANC_OVERRIDE;
    virtual IFlexiblePointerTracker* CreateEditionTracker(bool isCreation,
                                                          const PointerEvent& e) ORTHANC_OVERRIDE;
    virtual IMemento* CreateMemento() const ORTHANC_OVERRIDE;
    virtual void SetMemento(const IMemento& memento) ORTHANC_OVERRIDE;
    virtual std::string GetDescription() ORTHANC_OVERRIDE;

    enum AngleHighlightArea
    {
      AngleHighlightArea_None,
      AngleHighlightArea_Side1End,
      AngleHighlightArea_Side1,
      AngleHighlightArea_Side2End,
      AngleHighlightArea_Side2,
      AngleHighlightArea_Center
    };


    AngleHighlightArea AngleHitTest(const Scene2D& scene,
                                    ScenePoint2D p) const;

  private:
    class Tracker;

    explicit AngleMeasureTool(boost::weak_ptr<IViewport> viewport,
                              const boost::shared_ptr<ViewportController>& controller);

    virtual void        RefreshScene() ORTHANC_OVERRIDE;
    void                RemoveFromScene();
    void                SetAngleHighlightArea(AngleHighlightArea area);

    /**
    The angle measure too arc  radius in *scene* coordinates

    Note: you might wonder why this is not part of the AngleMeasureTool itself,
    but we prefer to put all such constants in the same location, to ease 
    */
    static double GetAngleToolArcRadiusS(const Scene2D& scene);

    /**
       TODO Refactoring - Remove this?

    Distance between the top of the angle measuring tool and the center of 
    the label showing the actual measure, in *scene* coordinates
    */
    static double GetAngleTopTextLabelDistanceS(const Scene2D& scene);

  private:
    ScenePoint2D                    side1End_;
    ScenePoint2D                    side2End_;
    ScenePoint2D                    center_;
    boost::shared_ptr<LayerHolder>  layerHolder_;
    AngleHighlightArea              angleHighlightArea_;
  };
}
