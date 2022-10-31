/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include "../Messages/IObservable.h"
#include "Scene2D.h"
#include "../Scene2DViewport/IFlexiblePointerTracker.h"

namespace OrthancStone
{
  class AnnotationsSceneLayer : public IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, AnnotationAddedMessage, AnnotationsSceneLayer);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, AnnotationRemovedMessage, AnnotationsSceneLayer);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, AnnotationChangedMessage, AnnotationsSceneLayer);

    enum Tool
    {
      Tool_Edit,
      Tool_None,
      Tool_Segment,
      Tool_Angle,
      Tool_Circle,
      Tool_Remove,
      Tool_PixelProbe,
      Tool_RectangleProbe,
      Tool_EllipseProbe,
      Tool_TextAnnotation
    };

  private:
    class GeometricPrimitive;    
    class Handle;    
    class Segment;
    class Circle;    
    class Arc;
    class Text;
    class Ellipse;

    class Annotation;
    class ProbingAnnotation;
    class PixelProbeAnnotation;
    class SegmentAnnotation;
    class AngleAnnotation;
    class CircleAnnotation;
    class RectangleProbeAnnotation;
    class EllipseProbeAnnotation;
    
    class EditPrimitiveTracker;
    class CreateTwoHandlesTracker;
    class CreateAngleTracker;
    class CreatePixelProbeTracker;
    class RemoveTracker;

    typedef std::set<GeometricPrimitive*>  GeometricPrimitives;
    typedef std::set<Annotation*>          Annotations;
    typedef std::set<size_t>               SubLayers;

    Tool                 activeTool_;
    size_t               macroLayerIndex_;
    size_t               polylineSubLayer_;
    GeometricPrimitives  primitives_;
    Annotations          annotations_;
    SubLayers            subLayersToRemove_;
    Units                units_;
    int                  probedLayer_;

    void AddAnnotation(Annotation* annotation);
    
    void DeleteAnnotation(Annotation* annotation);

    void DeletePrimitive(GeometricPrimitive* primitive);
    
    void TagSubLayerToRemove(size_t subLayerIndex);
    
  public:
    explicit AnnotationsSceneLayer(size_t macroLayerIndex);
    
    ~AnnotationsSceneLayer()
    {
      Clear();
    }

    void Clear();

    void SetActiveTool(Tool tool)
    {
      activeTool_ = tool;
    }

    Tool GetActiveTool() const
    {
      return activeTool_;
    }

    void SetUnits(Units units);

    Units GetUnits() const
    {
      return units_;
    }

    void AddSegmentAnnotation(const ScenePoint2D& p1,
                              const ScenePoint2D& p2);

    void AddCircleAnnotation(const ScenePoint2D& p1,
                             const ScenePoint2D& p2);

    void AddAngleAnnotation(const ScenePoint2D& p1,
                            const ScenePoint2D& p2,
                            const ScenePoint2D& p3);

    void Render(Scene2D& scene);
    
    bool ClearHover();

    bool SetMouseHover(const ScenePoint2D& p /* expressed in canvas coordinates */,
                       const Scene2D& scene);

    IFlexiblePointerTracker* CreateTracker(const ScenePoint2D& p /* expressed in canvas coordinates */,
                                           const Scene2D& scene);
    
    void Serialize(Json::Value& target) const;
    
    void Unserialize(const Json::Value& serialized);

    void SetProbedLayer(int layer)
    {
      probedLayer_ = layer;
    }

    int GetProbedLayer() const
    {
      return probedLayer_;
    }
  };
}
