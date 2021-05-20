/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2021 Osimis S.A., Belgium
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


#include "SdlSimpleViewerApplication.h"
#include "../SdlHelpers.h"
#include "../../Common/SampleHelpers.h"

#include "../../../../OrthancStone/Sources/Loaders/GenericLoadersContext.h"
#include "../../../../OrthancStone/Sources/Scene2DViewport/AngleMeasureTool.h"
#include "../../../../OrthancStone/Sources/Scene2DViewport/LineMeasureTool.h"
#include "../../../../OrthancStone/Sources/Scene2DViewport/UndoStack.h"
#include "../../../../OrthancStone/Sources/StoneEnumerations.h"
#include "../../../../OrthancStone/Sources/StoneException.h"
#include "../../../../OrthancStone/Sources/StoneInitialization.h"
#include "../../../../OrthancStone/Sources/Viewport/DefaultViewportInteractor.h"
#include "../../../Platforms/Sdl/SdlViewport.h"

#include <EmbeddedResources.h>
#include <Compatibility.h>  // For std::unique_ptr<>
#include <OrthancException.h>

#include <boost/program_options.hpp>
#include <SDL.h>

#include <string>



#if 1
#include "../../../../OrthancStone/Sources/Scene2D/MacroSceneLayer.h"

#include <boost/math/constants/constants.hpp>

static const double HANDLE_SIZE = 10.0;
static const double PI = boost::math::constants::pi<double>();
    
namespace OrthancStone
{
  class AnnotationsOverlay : public boost::noncopyable
  {
  public:
    enum Tool
    {
      Tool_Edit,
      Tool_None,
      Tool_Segment,
      Tool_Angle,
      Tool_Circle,
      Tool_Erase
    };
    
  private:
    class Measure;
    
    class Primitive : public boost::noncopyable
    {
    private:
      bool      modified_;
      Measure&  parentMeasure_;
      Color     color_;
      Color     hoverColor_;
      bool      isHover_;
      int       depth_;

    public:
      Primitive(Measure& parentMeasure,
                int depth) :
        modified_(true),
        parentMeasure_(parentMeasure),
        color_(192, 192, 192),
        hoverColor_(0, 255, 0),
        isHover_(false),
        depth_(depth)
      {
      }
      
      virtual ~Primitive()
      {
      }

      Measure& GetParentMeasure() const
      {
        return parentMeasure_;
      }
      
      int GetDepth() const
      {
        return depth_;
      }

      void SetHover(bool hover)
      {
        if (hover != isHover_)
        {
          isHover_ = hover;
          modified_ = true;
        }
      }

      bool IsHover() const
      {
        return isHover_;
      }
      
      void SetModified(bool modified)
      {
        modified_ = modified;
      }
      
      bool IsModified() const
      {
        return modified_;
      }

      void SetColor(const Color& color)
      {
        SetModified(true);
        color_ = color;
      }

      void SetHoverColor(const Color& color)
      {
        SetModified(true);
        hoverColor_ = color;
      }

      const Color& GetColor() const
      {
        return color_;
      }

      const Color& GetHoverColor() const
      {
        return hoverColor_;
      }

      virtual bool IsHit(const ScenePoint2D& p,
                         const Scene2D& scene) const = 0;

      // Always called, even if not modified
      virtual void RenderPolylineLayer(PolylineSceneLayer& polyline,
                                       const Scene2D& scene) = 0;

      // Only called if modified
      virtual void RenderOtherLayers(MacroSceneLayer& macro,
                                     const Scene2D& scene) = 0;

      virtual void MovePreview(const ScenePoint2D& delta) = 0;

      virtual void MoveDone(const ScenePoint2D& delta) = 0;
    };
    

    class Measure : public boost::noncopyable
    {
    private:
      typedef std::list<Primitive*>  Primitives;
      
      AnnotationsOverlay&  that_;
      Primitives           primitives_;
      
    public:
      Measure(AnnotationsOverlay& that) :
        that_(that)
      {
        that.AddMeasure(this);
      }
      
      virtual ~Measure()
      {
        for (Primitives::iterator it = primitives_.begin(); it != primitives_.end(); ++it)
        {
          that_.DeletePrimitive(*it);
        }
      }

      Primitive* AddPrimitive(Primitive* primitive)
      {
        if (primitive == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
        else
        {
          assert(that_.primitives_.find(primitive) == that_.primitives_.end());
          primitives_.push_back(primitive);  // For automated deallocation
          that_.primitives_.insert(primitive);
          return primitive;
        }
      }

      template <typename T>
      T& AddTypedPrimitive(T* primitive)
      {
        AddPrimitive(primitive);
        return *primitive;
      }

      virtual void SignalMove(Primitive& primitive) = 0;
    };


    class Handle : public Primitive
    {
    private:
      ScenePoint2D  center_;
      ScenePoint2D  delta_;

    public:
      explicit Handle(Measure& parentMeasure,
                      const ScenePoint2D& center) :
        Primitive(parentMeasure, 0),  // Highest priority
        center_(center),
        delta_(0, 0)
      {
      }

      void SetSize(unsigned int size)
      {
        SetModified(true);
      }

      void SetCenter(const ScenePoint2D& center)
      {
        SetModified(true);
        center_ = center;
        delta_ = ScenePoint2D(0, 0);
      }

      ScenePoint2D GetCenter() const
      {
        return center_ + delta_;
      }

      virtual bool IsHit(const ScenePoint2D& p,
                         const Scene2D& scene) const
      {
        const double zoom = scene.GetSceneToCanvasTransform().ComputeZoom();

        double dx = (center_.GetX() + delta_.GetX() - p.GetX()) * zoom;
        double dy = (center_.GetY() + delta_.GetY() - p.GetY()) * zoom;

        return (std::abs(dx) <= HANDLE_SIZE / 2.0 &&
                std::abs(dy) <= HANDLE_SIZE / 2.0);
      }

      virtual void RenderPolylineLayer(PolylineSceneLayer& polyline,
                                       const Scene2D& scene) ORTHANC_OVERRIDE
      {
        const double zoom = scene.GetSceneToCanvasTransform().ComputeZoom();

        // TODO: take DPI into account 
        double x1 = center_.GetX() + delta_.GetX() - (HANDLE_SIZE / 2.0) / zoom;
        double y1 = center_.GetY() + delta_.GetY() - (HANDLE_SIZE / 2.0) / zoom;
        double x2 = center_.GetX() + delta_.GetX() + (HANDLE_SIZE / 2.0) / zoom;
        double y2 = center_.GetY() + delta_.GetY() + (HANDLE_SIZE / 2.0) / zoom;

        PolylineSceneLayer::Chain chain;
        chain.reserve(4);
        chain.push_back(ScenePoint2D(x1, y1));
        chain.push_back(ScenePoint2D(x2, y1));
        chain.push_back(ScenePoint2D(x2, y2));
        chain.push_back(ScenePoint2D(x1, y2));

        if (IsHover())
        {
          polyline.AddChain(chain, true /* closed */, GetHoverColor());
        }
        else
        {
          polyline.AddChain(chain, true /* closed */, GetColor());
        }
      }
      
      virtual void RenderOtherLayers(MacroSceneLayer& macro,
                                     const Scene2D& scene) ORTHANC_OVERRIDE
      {
      }

      virtual void MovePreview(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        SetModified(true);
        delta_ = delta;
        GetParentMeasure().SignalMove(*this);
      }

      virtual void MoveDone(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        SetModified(true);
        center_ = center_ + delta;
        delta_ = ScenePoint2D(0, 0);
        GetParentMeasure().SignalMove(*this);
      }
    };

    
    class Segment : public Primitive
    {
    private:
      ScenePoint2D  p1_;
      ScenePoint2D  p2_;
      ScenePoint2D  delta_;
      
    public:
      Segment(Measure& parentMeasure,
              const ScenePoint2D& p1,
              const ScenePoint2D& p2) :
        Primitive(parentMeasure, 1),  // Can only be selected if no handle matches
        p1_(p1),
        p2_(p2),
        delta_(0, 0)
      {
      }

      void SetPosition(const ScenePoint2D& p1,
                       const ScenePoint2D& p2)
      {
        SetModified(true);
        p1_ = p1;
        p2_ = p2;
        delta_ = ScenePoint2D(0, 0);
      }

      ScenePoint2D GetPosition1() const
      {
        return p1_ + delta_;
      }

      ScenePoint2D GetPosition2() const
      {
        return p2_ + delta_;
      }

      virtual bool IsHit(const ScenePoint2D& p,
                         const Scene2D& scene) const
      {
        const double zoom = scene.GetSceneToCanvasTransform().ComputeZoom();
        return (ScenePoint2D::SquaredDistancePtSegment(p1_ + delta_, p2_ + delta_, p) * zoom * zoom <=
                (HANDLE_SIZE / 2.0) * (HANDLE_SIZE / 2.0));
      }

      virtual void RenderPolylineLayer(PolylineSceneLayer& polyline,
                                       const Scene2D& scene) ORTHANC_OVERRIDE
      {
        PolylineSceneLayer::Chain chain;
        chain.reserve(2);
        chain.push_back(p1_ + delta_);
        chain.push_back(p2_ + delta_);

        if (IsHover())
        {
          polyline.AddChain(chain, false /* closed */, GetHoverColor());
        }
        else
        {
          polyline.AddChain(chain, false /* closed */, GetColor());
        }
      }
      
      virtual void RenderOtherLayers(MacroSceneLayer& macro,
                                     const Scene2D& scene) ORTHANC_OVERRIDE
      {
      }

      virtual void MovePreview(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        SetModified(true);
        delta_ = delta;
        GetParentMeasure().SignalMove(*this);
      }

      virtual void MoveDone(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        SetModified(true);
        p1_ = p1_ + delta;
        p2_ = p2_ + delta;
        delta_ = ScenePoint2D(0, 0);
        GetParentMeasure().SignalMove(*this);
      }
    };

    
    class Circle : public Primitive
    {
    private:
      ScenePoint2D  p1_;
      ScenePoint2D  p2_;
      
    public:
      Circle(Measure& parentMeasure,
             const ScenePoint2D& p1,
             const ScenePoint2D& p2) :
        Primitive(parentMeasure, 2),
        p1_(p1),
        p2_(p2)
      {
      }

      void SetPosition(const ScenePoint2D& p1,
                       const ScenePoint2D& p2)
      {
        SetModified(true);
        p1_ = p1;
        p2_ = p2;
      }

      ScenePoint2D GetPosition1() const
      {
        return p1_;
      }

      ScenePoint2D GetPosition2() const
      {
        return p2_;
      }

      virtual bool IsHit(const ScenePoint2D& p,
                         const Scene2D& scene) const
      {
        return false;
      }

      virtual void RenderPolylineLayer(PolylineSceneLayer& polyline,
                                       const Scene2D& scene) ORTHANC_OVERRIDE
      {
        static unsigned int NUM_SEGMENTS = 128;

        ScenePoint2D middle((p1_.GetX() + p2_.GetX()) / 2.0,
                            (p1_.GetY() + p2_.GetY()) / 2.0);
        
        const double radius = ScenePoint2D::DistancePtPt(middle, p1_);

        double increment = 2.0 * PI / static_cast<double>(NUM_SEGMENTS - 1);

        PolylineSceneLayer::Chain chain;
        chain.reserve(NUM_SEGMENTS);

        double theta = 0;
        for (unsigned int i = 0; i < NUM_SEGMENTS; i++)
        {
          chain.push_back(ScenePoint2D(middle.GetX() + radius * cos(theta),
                                       middle.GetY() + radius * sin(theta)));
          theta += increment;
        }
        
        if (IsHover())
        {
          polyline.AddChain(chain, false /* closed */, GetHoverColor());
        }
        else
        {
          polyline.AddChain(chain, false /* closed */, GetColor());
        }
      }
      
      virtual void RenderOtherLayers(MacroSceneLayer& macro,
                                     const Scene2D& scene) ORTHANC_OVERRIDE
      {
      }

      virtual void MovePreview(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);  // No hit is possible
      }

      virtual void MoveDone(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);  // No hit is possible
      }
    };

    
    class Arc : public Primitive
    {
    private:
      ScenePoint2D  start_;
      ScenePoint2D  middle_;
      ScenePoint2D  end_;
      double        radius_;  // in pixels

      void ComputeAngles(double& fullAngle,
                         double& startAngle,
                         double& endAngle) const
      {
        const double x1 = start_.GetX();
        const double y1 = start_.GetY();
        const double xc = middle_.GetX();
        const double yc = middle_.GetY();
        const double x2 = end_.GetX();
        const double y2 = end_.GetY();
        
        startAngle = atan2(y1 - yc, x1 - xc);
        endAngle = atan2(y2 - yc, x2 - xc);

        fullAngle = endAngle - startAngle;
        
        while (fullAngle < -PI)
        {
          fullAngle += 2.0 * PI;
        }
        
        while (fullAngle >= PI)
        {
          fullAngle -= 2.0 * PI;
        }
      }
      
    public:
      Arc(Measure& parentMeasure,
          const ScenePoint2D& start,
          const ScenePoint2D& middle,
          const ScenePoint2D& end) :
        Primitive(parentMeasure, 2),
        start_(start),
        middle_(middle),
        end_(end),
        radius_(20)
      {
      }

      double GetAngle() const
      {
        double fullAngle, startAngle, endAngle;
        ComputeAngles(fullAngle, startAngle, endAngle);
        return fullAngle;
      }

      void SetStart(const ScenePoint2D& p)
      {
        SetModified(true);
        start_ = p;
      }

      void SetMiddle(const ScenePoint2D& p)
      {
        SetModified(true);
        middle_ = p;
      }

      void SetEnd(const ScenePoint2D& p)
      {
        SetModified(true);
        end_ = p;
      }

      virtual bool IsHit(const ScenePoint2D& p,
                         const Scene2D& scene) const
      {
        return false;
      }

      virtual void RenderPolylineLayer(PolylineSceneLayer& polyline,
                                       const Scene2D& scene) ORTHANC_OVERRIDE
      {
        static unsigned int NUM_SEGMENTS = 64;

        const double radius = radius_ / scene.GetSceneToCanvasTransform().ComputeZoom();

        double fullAngle, startAngle, endAngle;
        ComputeAngles(fullAngle, startAngle, endAngle);

        double increment = fullAngle / static_cast<double>(NUM_SEGMENTS - 1);

        PolylineSceneLayer::Chain chain;
        chain.reserve(NUM_SEGMENTS);

        double theta = startAngle;
        for (unsigned int i = 0; i < NUM_SEGMENTS; i++)
        {
          chain.push_back(ScenePoint2D(middle_.GetX() + radius * cos(theta),
                                       middle_.GetY() + radius * sin(theta)));
          theta += increment;
        }
        
        if (IsHover())
        {
          polyline.AddChain(chain, false /* closed */, GetHoverColor());
        }
        else
        {
          polyline.AddChain(chain, false /* closed */, GetColor());
        }
      }
      
      virtual void RenderOtherLayers(MacroSceneLayer& macro,
                                     const Scene2D& scene) ORTHANC_OVERRIDE
      {
      }

      virtual void MovePreview(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);  // No hit is possible
      }

      virtual void MoveDone(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);  // No hit is possible
      }
    };

    
    class Text : public Primitive
    {
    private:
      AnnotationsOverlay&              that_;
      bool                             first_;
      size_t                           subLayer_;
      std::unique_ptr<TextSceneLayer>  content_;

    public:
      Text(AnnotationsOverlay& that,
           Measure& parentMeasure) :
        Primitive(parentMeasure, 2),
        that_(that),
        first_(true)
      {
      }

      virtual ~Text()
      {
        if (content_.get() != NULL)
        {
          that_.TagSubLayerToRemove(subLayer_);
        }
      }
      
      void SetContent(const TextSceneLayer& content)
      {
        SetModified(true);
        content_.reset(dynamic_cast<TextSceneLayer*>(content.Clone()));
      }        

      virtual bool IsHit(const ScenePoint2D& p,
                         const Scene2D& scene) const
      {
        return false;
      }

      virtual void RenderPolylineLayer(PolylineSceneLayer& polyline,
                                       const Scene2D& scene) ORTHANC_OVERRIDE
      {
      }
      
      virtual void RenderOtherLayers(MacroSceneLayer& macro,
                                     const Scene2D& scene) ORTHANC_OVERRIDE
      {
        if (content_.get() != NULL)
        {
          std::unique_ptr<TextSceneLayer> layer(reinterpret_cast<TextSceneLayer*>(content_->Clone()));

          layer->SetColor(IsHover() ? GetHoverColor() : GetColor());
          
          if (first_)
          {
            subLayer_ = macro.AddLayer(layer.release());
            first_ = false;
          }
          else
          {
            macro.UpdateLayer(subLayer_, layer.release());
          }
        }
      }

      virtual void MovePreview(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);  // No hit is possible
      }

      virtual void MoveDone(const ScenePoint2D& delta) ORTHANC_OVERRIDE
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);  // No hit is possible
      }
    };


    class EditPrimitiveTracker : public IFlexiblePointerTracker
    {
    private:
      Primitive&         primitive_;
      ScenePoint2D       sceneClick_;
      AffineTransform2D  canvasToScene_;
      bool               alive_;
      
    public:
      EditPrimitiveTracker(Primitive& primitive,
                           const ScenePoint2D& sceneClick,
                           const AffineTransform2D& canvasToScene) :
        primitive_(primitive),
        sceneClick_(sceneClick),
        canvasToScene_(canvasToScene),
        alive_(true)
      {
      }

      virtual void PointerMove(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        primitive_.MovePreview(event.GetMainPosition().Apply(canvasToScene_) - sceneClick_);
      }
      
      virtual void PointerUp(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        primitive_.MoveDone(event.GetMainPosition().Apply(canvasToScene_) - sceneClick_);
        alive_ = false;
      }

      virtual void PointerDown(const PointerEvent& event) ORTHANC_OVERRIDE
      {
      }

      virtual bool IsAlive() const ORTHANC_OVERRIDE
      {
        return alive_;
      }

      virtual void Cancel() ORTHANC_OVERRIDE
      {
        primitive_.MoveDone(ScenePoint2D(0, 0));
      }
    };


    class SegmentMeasure : public Measure
    {
    private:
      bool      showLabel_;
      Handle&   handle1_;
      Handle&   handle2_;
      Segment&  segment_;
      Text&     label_;

      void UpdateLabel()
      {
        if (showLabel_)
        {
          TextSceneLayer content;

          double x1 = handle1_.GetCenter().GetX();
          double y1 = handle1_.GetCenter().GetY();
          double x2 = handle2_.GetCenter().GetX();
          double y2 = handle2_.GetCenter().GetY();
        
          // Put the label to the right of the right-most handle
          if (x1 < x2)
          {
            content.SetPosition(x2, y2);
          }
          else
          {
            content.SetPosition(x1, y1);
          }

          content.SetAnchor(BitmapAnchor_CenterLeft);
          content.SetBorder(10);

          double dx = x1 - x2;
          double dy = y1 - y2;
          char buf[32];
          sprintf(buf, "%0.2f cm", sqrt(dx * dx + dy * dy) / 10.0);
          content.SetText(buf);

          label_.SetContent(content);
        }
      }

    public:
      SegmentMeasure(AnnotationsOverlay& that,
                     bool showLabel,
                     const ScenePoint2D& p1,
                     const ScenePoint2D& p2) :
        Measure(that),
        showLabel_(showLabel),
        handle1_(AddTypedPrimitive<Handle>(new Handle(*this, p1))),
        handle2_(AddTypedPrimitive<Handle>(new Handle(*this, p2))),
        segment_(AddTypedPrimitive<Segment>(new Segment(*this, p1, p2))),
        label_(AddTypedPrimitive<Text>(new Text(that, *this)))
      {
        label_.SetColor(Color(255, 0, 0));
        UpdateLabel();
      }

      Handle& GetHandle1() const
      {
        return handle1_;
      }

      Handle& GetHandle2() const
      {
        return handle2_;
      }

      virtual void SignalMove(Primitive& primitive) ORTHANC_OVERRIDE
      {
        if (&primitive == &handle1_ ||
            &primitive == &handle2_)
        {
          segment_.SetPosition(handle1_.GetCenter(), handle2_.GetCenter());
        }
        else if (&primitive == &segment_)
        {
          handle1_.SetCenter(segment_.GetPosition1());
          handle2_.SetCenter(segment_.GetPosition2());
        }
        
        UpdateLabel();
      }
    };

    
    class AngleMeasure : public Measure
    {
    private:
      Handle&   startHandle_;
      Handle&   middleHandle_;
      Handle&   endHandle_;
      Segment&  segment1_;
      Segment&  segment2_;
      Arc&      arc_;
      Text&     label_;

      void UpdateLabel()
      {
        TextSceneLayer content;

        const double x1 = startHandle_.GetCenter().GetX();
        const double x2 = middleHandle_.GetCenter().GetX();
        const double y2 = middleHandle_.GetCenter().GetY();
        const double x3 = endHandle_.GetCenter().GetX();
        
        if (x2 < x1 &&
            x2 < x3)
        {
          content.SetAnchor(BitmapAnchor_CenterRight);
        }
        else
        {
          content.SetAnchor(BitmapAnchor_CenterLeft);
        }

        content.SetPosition(x2, y2);
        content.SetBorder(10);

        char buf[32];
        sprintf(buf, "%.01f%c%c", std::abs(arc_.GetAngle()) / PI * 180.0,
                0xc2, 0xb0 /* two bytes corresponding to degree symbol in UTF-8 */);
        content.SetText(buf);

        label_.SetContent(content);
      }

    public:
      AngleMeasure(AnnotationsOverlay& that,
                   const ScenePoint2D& start,
                   const ScenePoint2D& middle,
                   const ScenePoint2D& end) :
        Measure(that),
        startHandle_(AddTypedPrimitive<Handle>(new Handle(*this, start))),
        middleHandle_(AddTypedPrimitive<Handle>(new Handle(*this, middle))),
        endHandle_(AddTypedPrimitive<Handle>(new Handle(*this, end))),
        segment1_(AddTypedPrimitive<Segment>(new Segment(*this, start, middle))),
        segment2_(AddTypedPrimitive<Segment>(new Segment(*this, middle, end))),
        arc_(AddTypedPrimitive<Arc>(new Arc(*this, start, middle, end))),
        label_(AddTypedPrimitive<Text>(new Text(that, *this)))
      {
        label_.SetColor(Color(255, 0, 0));
        UpdateLabel();
      }

      Handle& GetEndHandle() const
      {
        return endHandle_;
      }

      virtual void SignalMove(Primitive& primitive) ORTHANC_OVERRIDE
      {
        if (&primitive == &startHandle_)
        {
          segment1_.SetPosition(startHandle_.GetCenter(), middleHandle_.GetCenter());
          arc_.SetStart(startHandle_.GetCenter());
        }
        else if (&primitive == &middleHandle_)
        {
          segment1_.SetPosition(startHandle_.GetCenter(), middleHandle_.GetCenter());
          segment2_.SetPosition(middleHandle_.GetCenter(), endHandle_.GetCenter());
          arc_.SetMiddle(middleHandle_.GetCenter());
        }
        else if (&primitive == &endHandle_)
        {
          segment2_.SetPosition(middleHandle_.GetCenter(), endHandle_.GetCenter());
          arc_.SetEnd(endHandle_.GetCenter());
        }
        else if (&primitive == &segment1_)
        {
          startHandle_.SetCenter(segment1_.GetPosition1());
          middleHandle_.SetCenter(segment1_.GetPosition2());
          segment2_.SetPosition(segment1_.GetPosition2(), segment2_.GetPosition2());
          arc_.SetStart(segment1_.GetPosition1());
          arc_.SetMiddle(segment1_.GetPosition2());
        }
        else if (&primitive == &segment2_)
        {
          middleHandle_.SetCenter(segment2_.GetPosition1());
          endHandle_.SetCenter(segment2_.GetPosition2());
          segment1_.SetPosition(segment1_.GetPosition1(), segment2_.GetPosition1());
          arc_.SetMiddle(segment2_.GetPosition1());
          arc_.SetEnd(segment2_.GetPosition2());
        }

        UpdateLabel();
      }
    };

    
    class CircleMeasure : public Measure
    {
    private:
      Handle&   handle1_;
      Handle&   handle2_;
      Segment&  segment_;
      Circle&   circle_;
      Text&     label_;

      void UpdateLabel()
      {
        TextSceneLayer content;

        double x1 = handle1_.GetCenter().GetX();
        double y1 = handle1_.GetCenter().GetY();
        double x2 = handle2_.GetCenter().GetX();
        double y2 = handle2_.GetCenter().GetY();
        
        // Put the label to the right of the right-most handle
        if (x1 < x2)
        {
          content.SetPosition(x2, y2);
        }
        else
        {
          content.SetPosition(x1, y1);
        }

        content.SetAnchor(BitmapAnchor_CenterLeft);
        content.SetBorder(10);

        double dx = x1 - x2;
        double dy = y1 - y2;
        double diameter = sqrt(dx * dx + dy * dy);  // in millimeters

        double area = PI * diameter * diameter / 4.0;
        
        char buf[32];
        sprintf(buf, "%0.2f cm\n%0.2f cm%c%c",
                diameter / 10.0,
                area / 100.0,
                0xc2, 0xb2 /* two bytes corresponding to two power in UTF-8 */);
        content.SetText(buf);

        label_.SetContent(content);
      }

    public:
      CircleMeasure(AnnotationsOverlay& that,
                    const ScenePoint2D& p1,
                    const ScenePoint2D& p2) :
        Measure(that),
        handle1_(AddTypedPrimitive<Handle>(new Handle(*this, p1))),
        handle2_(AddTypedPrimitive<Handle>(new Handle(*this, p2))),
        segment_(AddTypedPrimitive<Segment>(new Segment(*this, p1, p2))),
        circle_(AddTypedPrimitive<Circle>(new Circle(*this, p1, p2))),
        label_(AddTypedPrimitive<Text>(new Text(that, *this)))
      {
        label_.SetColor(Color(255, 0, 0));
        UpdateLabel();
      }

      Handle& GetHandle2() const
      {
        return handle2_;
      }

      virtual void SignalMove(Primitive& primitive) ORTHANC_OVERRIDE
      {
        if (&primitive == &handle1_ ||
            &primitive == &handle2_)
        {
          segment_.SetPosition(handle1_.GetCenter(), handle2_.GetCenter());
          circle_.SetPosition(handle1_.GetCenter(), handle2_.GetCenter());          
        }
        else if (&primitive == &segment_)
        {
          handle1_.SetCenter(segment_.GetPosition1());
          handle2_.SetCenter(segment_.GetPosition2());
          circle_.SetPosition(segment_.GetPosition1(), segment_.GetPosition2());
        }
        
        UpdateLabel();
      }
    };

    
    class CreateSegmentOrCircleTracker : public IFlexiblePointerTracker
    {
    private:
      AnnotationsOverlay&  that_;
      Measure*             measure_;
      AffineTransform2D    canvasToScene_;
      Handle*              handle2_;
      
    public:
      CreateSegmentOrCircleTracker(AnnotationsOverlay& that,
                                   bool isCircle,
                                   const ScenePoint2D& sceneClick,
                                   const AffineTransform2D& canvasToScene) :
        that_(that),
        measure_(NULL),
        canvasToScene_(canvasToScene),
        handle2_(NULL)
      {
        if (isCircle)
        {
          measure_ = new CircleMeasure(that, sceneClick, sceneClick);
          handle2_ = &dynamic_cast<CircleMeasure*>(measure_)->GetHandle2();
        }
        else
        {
          measure_ = new SegmentMeasure(that, true /* show label */, sceneClick, sceneClick);
          handle2_ = &dynamic_cast<SegmentMeasure*>(measure_)->GetHandle2();
        }
        
        assert(measure_ != NULL &&
               handle2_ != NULL);
      }

      virtual void PointerMove(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        if (measure_ != NULL)
        {
          assert(handle2_ != NULL);
          handle2_->SetCenter(event.GetMainPosition().Apply(canvasToScene_));
          measure_->SignalMove(*handle2_);
        }
      }
      
      virtual void PointerUp(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        measure_ = NULL;  // IsAlive() becomes false
      }

      virtual void PointerDown(const PointerEvent& event) ORTHANC_OVERRIDE
      {
      }

      virtual bool IsAlive() const ORTHANC_OVERRIDE
      {
        return (measure_ != NULL);
      }

      virtual void Cancel() ORTHANC_OVERRIDE
      {
        if (measure_ != NULL)
        {
          that_.DeleteMeasure(measure_);
          measure_ = NULL;
        }
      }
    };


    class CreateAngleTracker : public IFlexiblePointerTracker
    {
    private:
      AnnotationsOverlay&  that_;
      SegmentMeasure*      segment_;
      AngleMeasure*        angle_;
      AffineTransform2D    canvasToScene_;
      
    public:
      CreateAngleTracker(AnnotationsOverlay& that,
                         const ScenePoint2D& sceneClick,
                         const AffineTransform2D& canvasToScene) :
        that_(that),
        segment_(NULL),
        angle_(NULL),
        canvasToScene_(canvasToScene)
      {
        segment_ = new SegmentMeasure(that, false /* no length label */, sceneClick, sceneClick);
      }

      virtual void PointerMove(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        if (segment_ != NULL)
        {
          segment_->GetHandle2().SetCenter(event.GetMainPosition().Apply(canvasToScene_));
          segment_->SignalMove(segment_->GetHandle2());
        }

        if (angle_ != NULL)
        {
          angle_->GetEndHandle().SetCenter(event.GetMainPosition().Apply(canvasToScene_));
          angle_->SignalMove(angle_->GetEndHandle());
        }
      }
      
      virtual void PointerUp(const PointerEvent& event) ORTHANC_OVERRIDE
      {
        if (segment_ != NULL)
        {
          // End of first step: The first segment is available, now create the angle

          angle_ = new AngleMeasure(that_, segment_->GetHandle1().GetCenter(),
                                    segment_->GetHandle2().GetCenter(),
                                    segment_->GetHandle2().GetCenter());
          
          that_.DeleteMeasure(segment_);
          segment_ = NULL;
        }
        else
        {
          angle_ = NULL;  // IsAlive() becomes false
        }
      }

      virtual void PointerDown(const PointerEvent& event) ORTHANC_OVERRIDE
      {
      }

      virtual bool IsAlive() const ORTHANC_OVERRIDE
      {
        return (segment_ != NULL ||
                angle_ != NULL);
      }

      virtual void Cancel() ORTHANC_OVERRIDE
      {
        if (segment_ != NULL)
        {
          that_.DeleteMeasure(segment_);
          segment_ = NULL;
        }

        if (angle_ != NULL)
        {
          that_.DeleteMeasure(angle_);
          angle_ = NULL;
        }
      }
    };


    // Dummy tracker that is only used for deletion, in order to warn
    // the caller that the mouse action was taken into consideration
    class EraseTracker : public IFlexiblePointerTracker
    {
    public:
      EraseTracker()
      {
      }

      virtual void PointerMove(const PointerEvent& event) ORTHANC_OVERRIDE
      {
      }
      
      virtual void PointerUp(const PointerEvent& event) ORTHANC_OVERRIDE
      {
      }

      virtual void PointerDown(const PointerEvent& event) ORTHANC_OVERRIDE
      {
      }

      virtual bool IsAlive() const ORTHANC_OVERRIDE
      {
        return false;
      }

      virtual void Cancel() ORTHANC_OVERRIDE
      {
      }
    };


    typedef std::set<Primitive*>  Primitives;
    typedef std::set<Measure*>    Measures;
    typedef std::set<size_t>      SubLayers;

    Tool        activeTool_;
    size_t      macroLayerIndex_;
    size_t      polylineSubLayer_;
    Primitives  primitives_;
    Measures    measures_;
    SubLayers   subLayersToRemove_;

    void AddMeasure(Measure* measure)
    {
      assert(measure != NULL);
      assert(measures_.find(measure) == measures_.end());
      measures_.insert(measure);
    }

    void DeleteMeasure(Measure* measure)
    {
      if (measure != NULL)
      {
        assert(measures_.find(measure) != measures_.end());
        measures_.erase(measure);
        delete measure;
      }
    }

    void DeletePrimitive(Primitive* primitive)
    {
      if (primitive != NULL)
      {
        assert(primitives_.find(primitive) != primitives_.end());
        primitives_.erase(primitive);
        delete primitive;
      }
    }

    void TagSubLayerToRemove(size_t subLayerIndex)
    {
      assert(subLayersToRemove_.find(subLayerIndex) == subLayersToRemove_.end());
      subLayersToRemove_.insert(subLayerIndex);
    }
    
  public:
    AnnotationsOverlay(size_t macroLayerIndex) :
      activeTool_(Tool_Edit),
      macroLayerIndex_(macroLayerIndex),
      polylineSubLayer_(0)  // dummy initialization
    {
      measures_.insert(new SegmentMeasure(*this, true /* show label */, ScenePoint2D(0, 0), ScenePoint2D(100, 100)));
      measures_.insert(new AngleMeasure(*this, ScenePoint2D(100, 50), ScenePoint2D(150, 40), ScenePoint2D(200, 50)));
      measures_.insert(new CircleMeasure(*this, ScenePoint2D(50, 200), ScenePoint2D(100, 250)));
    }
    
    ~AnnotationsOverlay()
    {
      for (Measures::iterator it = measures_.begin(); it != measures_.end(); ++it)
      {
        assert(*it != NULL);
        delete *it;
      }

      measures_.clear();
    }

    void SetActiveTool(Tool tool)
    {
      activeTool_ = tool;
    }

    Tool GetActiveTool() const
    {
      return activeTool_;
    }

    void Render(Scene2D& scene)
    {
      MacroSceneLayer* macro = NULL;

      if (scene.HasLayer(macroLayerIndex_))
      {
        macro = &dynamic_cast<MacroSceneLayer&>(scene.GetLayer(macroLayerIndex_));
      }
      else
      {
        macro = &dynamic_cast<MacroSceneLayer&>(scene.SetLayer(macroLayerIndex_, new MacroSceneLayer));
        polylineSubLayer_ = macro->AddLayer(new PolylineSceneLayer);
      }

      for (SubLayers::const_iterator it = subLayersToRemove_.begin(); it != subLayersToRemove_.end(); ++it)
      {
        assert(macro->HasLayer(*it));
        macro->DeleteLayer(*it);
      }

      subLayersToRemove_.clear();

      std::unique_ptr<PolylineSceneLayer> polyline(new PolylineSceneLayer);

      for (Primitives::iterator it = primitives_.begin(); it != primitives_.end(); ++it)
      {
        assert(*it != NULL);
        Primitive& primitive = **it;        
        
        primitive.RenderPolylineLayer(*polyline, scene);

        if (primitive.IsModified())
        {
          primitive.RenderOtherLayers(*macro, scene);
          primitive.SetModified(false);
        }
      }

      macro->UpdateLayer(polylineSubLayer_, polyline.release());
    }

    bool ClearHover()
    {
      bool needsRefresh = false;
      
      for (Primitives::iterator it = primitives_.begin(); it != primitives_.end(); ++it)
      {
        assert(*it != NULL);
        if ((*it)->IsHover())
        {
          (*it)->SetHover(false);
          needsRefresh = true;
        }
      }

      return needsRefresh;
    }

    bool SetMouseHover(const ScenePoint2D& p /* expressed in canvas coordinates */,
                       const Scene2D& scene)
    {
      if (activeTool_ == Tool_None)
      {
        return ClearHover();
      }
      else
      {
        bool needsRefresh = false;
      
        const ScenePoint2D s = p.Apply(scene.GetCanvasToSceneTransform());
      
        for (Primitives::iterator it = primitives_.begin(); it != primitives_.end(); ++it)
        {
          assert(*it != NULL);
          bool hover = (*it)->IsHit(s, scene);

          if ((*it)->IsHover() != hover)
          {
            needsRefresh = true;
          }
        
          (*it)->SetHover(hover);
        }

        return needsRefresh;
      }
    }


    IFlexiblePointerTracker* CreateTracker(const ScenePoint2D& p /* expressed in canvas coordinates */,
                                           const Scene2D& scene)
    {
      if (activeTool_ == Tool_None)
      {
        return NULL;
      }
      else
      {
        const ScenePoint2D s = p.Apply(scene.GetCanvasToSceneTransform());

        Primitive* bestHit = NULL;
      
        for (Primitives::iterator it = primitives_.begin(); it != primitives_.end(); ++it)
        {
          assert(*it != NULL);
          if ((*it)->IsHit(s, scene))
          {
            if (bestHit == NULL ||
                bestHit->GetDepth() > (*it)->GetDepth())
            {
              bestHit = *it;
            }
          }
        }

        if (bestHit != NULL)
        {
          if (activeTool_ == Tool_Erase)
          {
            DeleteMeasure(&bestHit->GetParentMeasure());
            return new EraseTracker;
          }
          else
          {
            return new EditPrimitiveTracker(*bestHit, s, scene.GetCanvasToSceneTransform());
          }
        }
        else
        {
          switch (activeTool_)
          {
            case Tool_Segment:
              return new CreateSegmentOrCircleTracker(*this, false /* segment */, s, scene.GetCanvasToSceneTransform());

            case Tool_Circle:
              return new CreateSegmentOrCircleTracker(*this, true /* circle */, s, scene.GetCanvasToSceneTransform());

            case Tool_Angle:
              return new CreateAngleTracker(*this, s, scene.GetCanvasToSceneTransform());

            default:
              return NULL;
          }
        }
      }
    }
  };
}
#endif



std::string orthancUrl;
std::string instanceId;
int frameIndex = 0;


static void ProcessOptions(int argc, char* argv[])
{
  namespace po = boost::program_options;
  po::options_description desc("Usage");

  desc.add_options()
    ("loglevel", po::value<std::string>()->default_value("WARNING"),
     "You can choose WARNING, INFO or TRACE for the logging level: Errors and warnings will always be displayed. (default: WARNING)")

    ("orthanc", po::value<std::string>()->default_value("http://localhost:8042"),
     "Base URL of the Orthanc instance")

    ("instance", po::value<std::string>()->default_value("285dece8-e1956b38-cdc7d084-6ce3371e-536a9ffc"),
     "Orthanc ID of the instance to display")

    ("frame_index", po::value<int>()->default_value(0),
     "The zero-based index of the frame (for multi-frame instances)")
    ;

  std::cout << desc << std::endl;

  std::cout << std::endl << "Keyboard shorcuts:" << std::endl
            << "  a\tEnable/disable the angle measure tool" << std::endl
            << "  f\tToggle fullscreen display" << std::endl
            << "  l\tEnable/disable the line measure tool" << std::endl
            << "  q\tExit" << std::endl
            << "  r\tRedo the last edit to the measure tools" << std::endl
            << "  s\tFit the viewpoint to the image" << std::endl
            << "  u\tUndo the last edit to the measure tools" << std::endl
            << std::endl << "Mouse buttons:" << std::endl
            << "  left  \tChange windowing, or edit measure" << std::endl
            << "  center\tMove the viewpoint, or edit measure" << std::endl
            << "  right \tZoom, or edit measure" << std::endl
            << std::endl;

  po::variables_map vm;
  try
  {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  }
  catch (std::exception& e)
  {
    std::cerr << "Please check your command line options! (\"" << e.what() << "\")" << std::endl;
  }

  if (vm.count("loglevel") > 0)
  {
    std::string logLevel = vm["loglevel"].as<std::string>();
    OrthancStoneHelpers::SetLogLevel(logLevel);
  }

  if (vm.count("orthanc") > 0)
  {
    // maybe check URL validity here
    orthancUrl = vm["orthanc"].as<std::string>();
  }

  if (vm.count("instance") > 0)
  {
    instanceId = vm["instance"].as<std::string>();
  }

  if (vm.count("frame_index") > 0)
  {
    frameIndex = vm["frame_index"].as<int>();
  }
}


enum ActiveTool
{
  ActiveTool_None,
  ActiveTool_Line,
  ActiveTool_Angle
};


/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  try
  {
    OrthancStone::StoneInitialize();
    OrthancStone::SdlWindow::GlobalInitialize();
    
    ProcessOptions(argc, argv);

    //Orthanc::Logging::EnableInfoLevel(true);
    //Orthanc::Logging::EnableTraceLevel(true);

    {

#if 1
      boost::shared_ptr<OrthancStone::SdlViewport> viewport =
        OrthancStone::SdlOpenGLViewport::Create("Stone of Orthanc", 800, 600);
#else
      boost::shared_ptr<OrthancStone::SdlViewport> viewport =
        OrthancStone::SdlCairoViewport::Create("Stone of Orthanc", 800, 600);
#endif

      boost::shared_ptr<OrthancStone::UndoStack> undoStack(new OrthancStone::UndoStack);

      OrthancStone::GenericLoadersContext context(1, 4, 1);

      Orthanc::WebServiceParameters orthancWebService;
      orthancWebService.SetUrl(orthancUrl);
      context.SetOrthancParameters(orthancWebService);

      context.StartOracle();

      {
        {
          std::string font;
          Orthanc::EmbeddedResources::GetFileResource(font, Orthanc::EmbeddedResources::UBUNTU_FONT);
          
          std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
          lock->GetCompositor().SetFont(0, font, 16, Orthanc::Encoding_Latin1);
          lock->GetController().SetUndoStack(undoStack);
        }

        ActiveTool activeTool = ActiveTool_None;

        boost::shared_ptr<OrthancStone::LineMeasureTool> lineMeasureTool(OrthancStone::LineMeasureTool::Create(viewport));
        bool lineMeasureFirst = true;
        lineMeasureTool->Disable();

        boost::shared_ptr<OrthancStone::AngleMeasureTool> angleMeasureTool(OrthancStone::AngleMeasureTool::Create(viewport));
        bool angleMeasureFirst = true;
        angleMeasureTool->Disable();

        OrthancStone::AnnotationsOverlay overlay(10);
        overlay.SetActiveTool(OrthancStone::AnnotationsOverlay::Tool_Angle);

        boost::shared_ptr<SdlSimpleViewerApplication> application(
          SdlSimpleViewerApplication::Create(context, viewport));

        OrthancStone::DicomSource source;

        application->LoadOrthancFrame(source, instanceId, frameIndex);

        OrthancStone::DefaultViewportInteractor interactor;
        interactor.SetWindowingLayer(0);

        {
          int scancodeCount = 0;
          const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

          bool stop = false;
          while (!stop)
          {
            bool paint = false;
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
              if (event.type == SDL_QUIT)
              {
                stop = true;
                break;
              }
              else if (viewport->IsRefreshEvent(event))
              {
                paint = true;
              }
              else if (event.type == SDL_WINDOWEVENT &&
                       (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                        event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
              {
                viewport->UpdateSize(event.window.data1, event.window.data2);
              }
              else if (event.type == SDL_WINDOWEVENT &&
                       (event.window.event == SDL_WINDOWEVENT_SHOWN ||
                        event.window.event == SDL_WINDOWEVENT_EXPOSED))
              {
                std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                lock->RefreshCanvasSize();
              }
              else if (event.type == SDL_KEYDOWN &&
                       event.key.repeat == 0 /* Ignore key bounce */)
              {
                switch (event.key.keysym.sym)
                {
                  case SDLK_f:
                    viewport->ToggleMaximize();
                    break;

                  case SDLK_s:
                    application->FitContent();
                    break;

                  case SDLK_q:
                    stop = true;
                    break;

                  case SDLK_u:
                  {
                    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                    if (lock->GetController().CanUndo())
                    {
                      lock->GetController().Undo();
                    }
                    break;
                  }

                  case SDLK_r:
                  {
                    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                    if (lock->GetController().CanRedo())
                    {
                      lock->GetController().Redo();
                    }
                    break;
                  }

                  case SDLK_l:
                    if (activeTool == ActiveTool_Line)
                    {
                      lineMeasureTool->Disable();
                      activeTool = ActiveTool_None;
                    }
                    else
                    {
                      if (lineMeasureFirst)
                      {
                        std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                        OrthancStone::Extent2D extent;
                        lock->GetController().GetScene().GetBoundingBox(extent);
                        if (!extent.IsEmpty())
                        {
                          OrthancStone::ScenePoint2D p(extent.GetCenterX(), extent.GetCenterY());
                          lineMeasureTool->Set(p, p);
                        }
                        lineMeasureFirst = false;
                      }
                      
                      lineMeasureTool->Enable();
                      angleMeasureTool->Disable();
                      activeTool = ActiveTool_Line;
                    }
                    break;

                  case SDLK_a:
                    if (activeTool == ActiveTool_Angle)
                    {
                      angleMeasureTool->Disable();
                      activeTool = ActiveTool_None;
                    }
                    else
                    {
                      if (angleMeasureFirst)
                      {
                        std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                        OrthancStone::Extent2D extent;
                        lock->GetController().GetScene().GetBoundingBox(extent);
                        if (!extent.IsEmpty())
                        {
                          OrthancStone::ScenePoint2D p1(1.0 * extent.GetX1() / 3.0 + 2.0 * extent.GetX2() / 3.0,
                                                        2.0 * extent.GetY1() / 3.0 + 2.0 * extent.GetY2() / 3.0);
                          OrthancStone::ScenePoint2D p2(1.0 * extent.GetX1() / 2.0 + 1.0 * extent.GetX2() / 2.0,
                                                        1.0 * extent.GetY1() / 3.0 + 1.0 * extent.GetY2() / 3.0);
                          OrthancStone::ScenePoint2D p3(2.0 * extent.GetX1() / 3.0 + 1.0 * extent.GetX2() / 3.0,
                                                        2.0 * extent.GetY1() / 3.0 + 2.0 * extent.GetY2() / 3.0);
                          angleMeasureTool->SetSide1End(p1);
                          angleMeasureTool->SetCenter(p2);
                          angleMeasureTool->SetSide2End(p3);
                        }
                        angleMeasureFirst = false;
                      }
                      
                      lineMeasureTool->Disable();
                      angleMeasureTool->Enable();
                      activeTool = ActiveTool_Angle;
                    }
                    break;

                  default:
                    break;
                }
              }
              else if (event.type == SDL_MOUSEBUTTONDOWN ||
                       event.type == SDL_MOUSEMOTION ||
                       event.type == SDL_MOUSEBUTTONUP)
              {
                std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                if (lock->HasCompositor())
                {
                  OrthancStone::PointerEvent p;
                  OrthancStoneHelpers::GetPointerEvent(p, lock->GetCompositor(),
                                                       event, keyboardState, scancodeCount);

                  switch (event.type)
                  {
                    case SDL_MOUSEBUTTONDOWN:
                    {
                      boost::shared_ptr<OrthancStone::IFlexiblePointerTracker> t;

                      t.reset(overlay.CreateTracker(p.GetMainPosition(), lock->GetController().GetScene()));
                      if (t.get() == NULL)
                      {
                        switch (activeTool)
                        {
                          case ActiveTool_Angle:
                            t = angleMeasureTool->CreateEditionTracker(p);
                            break;

                          case ActiveTool_Line:
                            t = lineMeasureTool->CreateEditionTracker(p);
                            break;

                          case ActiveTool_None:
                            break;
                          
                          default:
                            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
                        }
                      }
                      
                      if (t.get() != NULL)
                      {
                        lock->GetController().AcquireActiveTracker(t);
                      }
                      else
                      {
                        lock->GetController().HandleMousePress(interactor, p,
                                                               lock->GetCompositor().GetCanvasWidth(),
                                                               lock->GetCompositor().GetCanvasHeight());
                      }
                      lock->Invalidate();
                      break;
                    }

                    case SDL_MOUSEMOTION:
                      if (lock->GetController().HandleMouseMove(p))
                      {
                        lock->Invalidate();
                        if (overlay.ClearHover())
                        {
                          paint = true;
                        }                          
                      }
                      else
                      {
                        if (overlay.SetMouseHover(p.GetMainPosition(), lock->GetController().GetScene()))
                        {
                          paint = true;
                        }
                      }
                      break;

                    case SDL_MOUSEBUTTONUP:
                      lock->GetController().HandleMouseRelease(p);
                      lock->Invalidate();
                      break;

                    default:
                      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
                  }
                }
              }
            }

            if (paint)
            {
              {
                std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport->Lock());
                overlay.Render(lock->GetController().GetScene());
              }
              
              viewport->Paint();
            }

            // Small delay to avoid using 100% of CPU
            SDL_Delay(1);
          }
        }
        context.StopOracle();
      }
    }

    OrthancStone::SdlWindow::GlobalFinalize();
    OrthancStone::StoneFinalize();
    return 0;
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "OrthancException: " << e.What();
    return -1;
  }
  catch (OrthancStone::StoneException& e)
  {
    LOG(ERROR) << "StoneException: " << e.What();
    return -1;
  }
  catch (std::runtime_error& e)
  {
    LOG(ERROR) << "Runtime error: " << e.what();
    return -1;
  }
  catch (...)
  {
    LOG(ERROR) << "Native exception";
    return -1;
  }
}
