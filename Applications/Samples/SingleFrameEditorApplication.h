/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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


#pragma once

#include "SampleApplicationBase.h"

#include "../../Framework/Radiography/RadiographyScene.h"
#include "../../Framework/Radiography/RadiographyWidget.h"

#include "../../Framework/Toolbox/UndoRedoStack.h"

#include <Core/Images/FontRegistry.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PamReader.h>
#include <Core/Images/PamWriter.h>
#include <Core/Images/PngWriter.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>


// Export using PAM is faster than using PNG, but requires Orthanc
// core >= 1.4.3
#define EXPORT_USING_PAM  1


#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>


namespace OrthancStone
{
  class RadiographySceneCommand : public UndoRedoStack::ICommand
  {
  private:
    RadiographyScene&  scene_;
    size_t             layer_;

  protected:
    virtual void UndoInternal(RadiographyLayer& layer) const = 0;

    virtual void RedoInternal(RadiographyLayer& layer) const = 0;

  public:
    RadiographySceneCommand(RadiographyScene& scene,
                            size_t layer) :
      scene_(scene),
      layer_(layer)
    {
    }

    RadiographySceneCommand(const RadiographyScene::LayerAccessor& accessor) :
      scene_(accessor.GetScene()),
      layer_(accessor.GetIndex())
    {
    }

    virtual void Undo() const
    {
      RadiographyScene::LayerAccessor accessor(scene_, layer_);

      if (accessor.IsValid())
      {
        UndoInternal(accessor.GetLayer());
      }
    }

    virtual void Redo() const
    {
      RadiographyScene::LayerAccessor accessor(scene_, layer_);

      if (accessor.IsValid())
      {
        RedoInternal(accessor.GetLayer());
      }
    }
  };


  class RadiographyLayerRotateTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                 undoRedoStack_;
    RadiographyScene::LayerAccessor  accessor_;
    double                         centerX_;
    double                         centerY_;
    double                         originalAngle_;
    double                         clickAngle_;
    bool                           roundAngles_;

    bool ComputeAngle(double& angle /* out */,
                      double sceneX,
                      double sceneY) const
    {
      Vector u;
      LinearAlgebra::AssignVector(u, sceneX - centerX_, sceneY - centerY_);

      double nu = boost::numeric::ublas::norm_2(u);

      if (!LinearAlgebra::IsCloseToZero(nu))
      {
        u /= nu;
        angle = atan2(u[1], u[0]);
        return true;
      }
      else
      {
        return false;
      }
    }


    class UndoRedoCommand : public RadiographySceneCommand
    {
    private:
      double  sourceAngle_;
      double  targetAngle_;

      static int ToDegrees(double angle)
      {
        return boost::math::iround(angle * 180.0 / boost::math::constants::pi<double>());
      }
      
    protected:
      virtual void UndoInternal(RadiographyLayer& layer) const
      {
        LOG(INFO) << "Undo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
        layer.SetAngle(sourceAngle_);
      }

      virtual void RedoInternal(RadiographyLayer& layer) const
      {
        LOG(INFO) << "Redo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
        layer.SetAngle(targetAngle_);
      }

    public:
      UndoRedoCommand(const RadiographyLayerRotateTracker& tracker) :
        RadiographySceneCommand(tracker.accessor_),
        sourceAngle_(tracker.originalAngle_),
        targetAngle_(tracker.accessor_.GetLayer().GetAngle())
      {
      }
    };

      
  public:
    RadiographyLayerRotateTracker(UndoRedoStack& undoRedoStack,
                                  RadiographyScene& scene,
                                  const ViewportGeometry& view,
                                  size_t layer,
                                  double x,
                                  double y,
                                  bool roundAngles) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      roundAngles_(roundAngles)
    {
      if (accessor_.IsValid())
      {
        accessor_.GetLayer().GetCenter(centerX_, centerY_);
        originalAngle_ = accessor_.GetLayer().GetAngle();

        double sceneX, sceneY;
        view.MapDisplayToScene(sceneX, sceneY, x, y);

        if (!ComputeAngle(clickAngle_, x, y))
        {
          accessor_.Invalidate();
        }
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      static const double ROUND_ANGLE = 15.0 / 180.0 * boost::math::constants::pi<double>(); 
        
      double angle;
        
      if (accessor_.IsValid() &&
          ComputeAngle(angle, sceneX, sceneY))
      {
        angle = angle - clickAngle_ + originalAngle_;

        if (roundAngles_)
        {
          angle = boost::math::round<double>((angle / ROUND_ANGLE) * ROUND_ANGLE);
        }
          
        accessor_.GetLayer().SetAngle(angle);
      }
    }
  };
    
    
  class RadiographyLayerMoveTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                   undoRedoStack_;
    RadiographyScene::LayerAccessor  accessor_;
    double                           clickX_;
    double                           clickY_;
    double                           panX_;
    double                           panY_;
    bool                             oneAxis_;

    class UndoRedoCommand : public RadiographySceneCommand
    {
    private:
      double  sourceX_;
      double  sourceY_;
      double  targetX_;
      double  targetY_;

    protected:
      virtual void UndoInternal(RadiographyLayer& layer) const
      {
        layer.SetPan(sourceX_, sourceY_);
      }

      virtual void RedoInternal(RadiographyLayer& layer) const
      {
        layer.SetPan(targetX_, targetY_);
      }

    public:
      UndoRedoCommand(const RadiographyLayerMoveTracker& tracker) :
        RadiographySceneCommand(tracker.accessor_),
        sourceX_(tracker.panX_),
        sourceY_(tracker.panY_),
        targetX_(tracker.accessor_.GetLayer().GetPanX()),
        targetY_(tracker.accessor_.GetLayer().GetPanY())
      {
      }
    };


  public:
    RadiographyLayerMoveTracker(UndoRedoStack& undoRedoStack,
                                RadiographyScene& scene,
                                size_t layer,
                                double x,
                                double y,
                                bool oneAxis) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      clickX_(x),
      clickY_(y),
      oneAxis_(oneAxis)
    {
      if (accessor_.IsValid())
      {
        panX_ = accessor_.GetLayer().GetPanX();
        panY_ = accessor_.GetLayer().GetPanY();
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      if (accessor_.IsValid())
      {
        double dx = sceneX - clickX_;
        double dy = sceneY - clickY_;

        if (oneAxis_)
        {
          if (fabs(dx) > fabs(dy))
          {
            accessor_.GetLayer().SetPan(dx + panX_, panY_);
          }
          else
          {
            accessor_.GetLayer().SetPan(panX_, dy + panY_);
          }
        }
        else
        {
          accessor_.GetLayer().SetPan(dx + panX_, dy + panY_);
        }
      }
    }
  };


  class RadiographyLayerCropTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                   undoRedoStack_;
    RadiographyScene::LayerAccessor  accessor_;
    Corner                           corner_;
    unsigned int                     cropX_;
    unsigned int                     cropY_;
    unsigned int                     cropWidth_;
    unsigned int                     cropHeight_;

    class UndoRedoCommand : public RadiographySceneCommand
    {
    private:
      unsigned int  sourceCropX_;
      unsigned int  sourceCropY_;
      unsigned int  sourceCropWidth_;
      unsigned int  sourceCropHeight_;
      unsigned int  targetCropX_;
      unsigned int  targetCropY_;
      unsigned int  targetCropWidth_;
      unsigned int  targetCropHeight_;

    protected:
      virtual void UndoInternal(RadiographyLayer& layer) const
      {
        layer.SetCrop(sourceCropX_, sourceCropY_, sourceCropWidth_, sourceCropHeight_);
      }

      virtual void RedoInternal(RadiographyLayer& layer) const
      {
        layer.SetCrop(targetCropX_, targetCropY_, targetCropWidth_, targetCropHeight_);
      }

    public:
      UndoRedoCommand(const RadiographyLayerCropTracker& tracker) :
        RadiographySceneCommand(tracker.accessor_),
        sourceCropX_(tracker.cropX_),
        sourceCropY_(tracker.cropY_),
        sourceCropWidth_(tracker.cropWidth_),
        sourceCropHeight_(tracker.cropHeight_)
      {
        tracker.accessor_.GetLayer().GetCrop(targetCropX_, targetCropY_,
                                             targetCropWidth_, targetCropHeight_);
      }
    };


  public:
    RadiographyLayerCropTracker(UndoRedoStack& undoRedoStack,
                                RadiographyScene& scene,
                                const ViewportGeometry& view,
                                size_t layer,
                                double x,
                                double y,
                                Corner corner) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      corner_(corner)
    {
      if (accessor_.IsValid())
      {
        accessor_.GetLayer().GetCrop(cropX_, cropY_, cropWidth_, cropHeight_);          
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      if (accessor_.IsValid())
      {
        unsigned int x, y;
        
        RadiographyLayer& layer = accessor_.GetLayer();
        if (layer.GetPixel(x, y, sceneX, sceneY))
        {
          unsigned int targetX, targetWidth;

          if (corner_ == Corner_TopLeft ||
              corner_ == Corner_BottomLeft)
          {
            targetX = std::min(x, cropX_ + cropWidth_);
            targetWidth = cropX_ + cropWidth_ - targetX;
          }
          else
          {
            targetX = cropX_;
            targetWidth = std::max(x, cropX_) - cropX_;
          }

          unsigned int targetY, targetHeight;

          if (corner_ == Corner_TopLeft ||
              corner_ == Corner_TopRight)
          {
            targetY = std::min(y, cropY_ + cropHeight_);
            targetHeight = cropY_ + cropHeight_ - targetY;
          }
          else
          {
            targetY = cropY_;
            targetHeight = std::max(y, cropY_) - cropY_;
          }

          layer.SetCrop(targetX, targetY, targetWidth, targetHeight);
        }
      }
    }
  };
    
    
  class RadiographyLayerResizeTracker : public IWorldSceneMouseTracker
  {
  private:
    UndoRedoStack&                   undoRedoStack_;
    RadiographyScene::LayerAccessor  accessor_;
    bool                             roundScaling_;
    double                           originalSpacingX_;
    double                           originalSpacingY_;
    double                           originalPanX_;
    double                           originalPanY_;
    Corner                           oppositeCorner_;
    double                           oppositeX_;
    double                           oppositeY_;
    double                           baseScaling_;

    static double ComputeDistance(double x1,
                                  double y1,
                                  double x2,
                                  double y2)
    {
      double dx = x1 - x2;
      double dy = y1 - y2;
      return sqrt(dx * dx + dy * dy);
    }
      
    class UndoRedoCommand : public RadiographySceneCommand
    {
    private:
      double   sourceSpacingX_;
      double   sourceSpacingY_;
      double   sourcePanX_;
      double   sourcePanY_;
      double   targetSpacingX_;
      double   targetSpacingY_;
      double   targetPanX_;
      double   targetPanY_;

    protected:
      virtual void UndoInternal(RadiographyLayer& layer) const
      {
        layer.SetPixelSpacing(sourceSpacingX_, sourceSpacingY_);
        layer.SetPan(sourcePanX_, sourcePanY_);
      }

      virtual void RedoInternal(RadiographyLayer& layer) const
      {
        layer.SetPixelSpacing(targetSpacingX_, targetSpacingY_);
        layer.SetPan(targetPanX_, targetPanY_);
      }

    public:
      UndoRedoCommand(const RadiographyLayerResizeTracker& tracker) :
        RadiographySceneCommand(tracker.accessor_),
        sourceSpacingX_(tracker.originalSpacingX_),
        sourceSpacingY_(tracker.originalSpacingY_),
        sourcePanX_(tracker.originalPanX_),
        sourcePanY_(tracker.originalPanY_),
        targetSpacingX_(tracker.accessor_.GetLayer().GetPixelSpacingX()),
        targetSpacingY_(tracker.accessor_.GetLayer().GetPixelSpacingY()),
        targetPanX_(tracker.accessor_.GetLayer().GetPanX()),
        targetPanY_(tracker.accessor_.GetLayer().GetPanY())
      {
      }
    };


  public:
    RadiographyLayerResizeTracker(UndoRedoStack& undoRedoStack,
                                  RadiographyScene& scene,
                                  size_t layer,
                                  double x,
                                  double y,
                                  Corner corner,
                                  bool roundScaling) :
      undoRedoStack_(undoRedoStack),
      accessor_(scene, layer),
      roundScaling_(roundScaling)
    {
      if (accessor_.IsValid() &&
          accessor_.GetLayer().IsResizeable())
      {
        originalSpacingX_ = accessor_.GetLayer().GetPixelSpacingX();
        originalSpacingY_ = accessor_.GetLayer().GetPixelSpacingY();
        originalPanX_ = accessor_.GetLayer().GetPanX();
        originalPanY_ = accessor_.GetLayer().GetPanY();

        switch (corner)
        {
          case Corner_TopLeft:
            oppositeCorner_ = Corner_BottomRight;
            break;

          case Corner_TopRight:
            oppositeCorner_ = Corner_BottomLeft;
            break;

          case Corner_BottomLeft:
            oppositeCorner_ = Corner_TopRight;
            break;

          case Corner_BottomRight:
            oppositeCorner_ = Corner_TopLeft;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }

        accessor_.GetLayer().GetCorner(oppositeX_, oppositeY_, oppositeCorner_);

        double d = ComputeDistance(x, y, oppositeX_, oppositeY_);
        if (d >= std::numeric_limits<float>::epsilon())
        {
          baseScaling_ = 1.0 / d;
        }
        else
        {
          // Avoid division by zero in extreme cases
          accessor_.Invalidate();
        }
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      if (accessor_.IsValid() &&
          accessor_.GetLayer().IsResizeable())
      {
        undoRedoStack_.Add(new UndoRedoCommand(*this));
      }
    }

    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      static const double ROUND_SCALING = 0.1;
        
      if (accessor_.IsValid() &&
          accessor_.GetLayer().IsResizeable())
      {
        double scaling = ComputeDistance(oppositeX_, oppositeY_, sceneX, sceneY) * baseScaling_;

        if (roundScaling_)
        {
          scaling = boost::math::round<double>((scaling / ROUND_SCALING) * ROUND_SCALING);
        }
          
        RadiographyLayer& layer = accessor_.GetLayer();
        layer.SetPixelSpacing(scaling * originalSpacingX_,
                              scaling * originalSpacingY_);

        // Keep the opposite corner at a fixed location
        double ox, oy;
        layer.GetCorner(ox, oy, oppositeCorner_);
        layer.SetPan(layer.GetPanX() + oppositeX_ - ox,
                     layer.GetPanY() + oppositeY_ - oy);
      }
    }
  };


  class RadiographyWindowingTracker : public IWorldSceneMouseTracker
  {   
  public:
    enum Action
    {
      Action_IncreaseWidth,
      Action_DecreaseWidth,
      Action_IncreaseCenter,
      Action_DecreaseCenter
    };
    
  private:
    UndoRedoStack&      undoRedoStack_;
    RadiographyScene&   scene_;
    int                 clickX_;
    int                 clickY_;
    Action              leftAction_;
    Action              rightAction_;
    Action              upAction_;
    Action              downAction_;
    float               strength_;
    float               sourceCenter_;
    float               sourceWidth_;

    static void ComputeAxisEffect(int& deltaCenter,
                                  int& deltaWidth,
                                  int delta,
                                  Action actionNegative,
                                  Action actionPositive)
    {
      if (delta < 0)
      {
        switch (actionNegative)
        {
          case Action_IncreaseWidth:
            deltaWidth = -delta;
            break;

          case Action_DecreaseWidth:
            deltaWidth = delta;
            break;

          case Action_IncreaseCenter:
            deltaCenter = -delta;
            break;

          case Action_DecreaseCenter:
            deltaCenter = delta;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }
      else if (delta > 0)
      {
        switch (actionPositive)
        {
          case Action_IncreaseWidth:
            deltaWidth = delta;
            break;

          case Action_DecreaseWidth:
            deltaWidth = -delta;
            break;

          case Action_IncreaseCenter:
            deltaCenter = delta;
            break;

          case Action_DecreaseCenter:
            deltaCenter = -delta;
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
      }
    }
    
    
    class UndoRedoCommand : public UndoRedoStack::ICommand
    {
    private:
      RadiographyScene&  scene_;
      float              sourceCenter_;
      float              sourceWidth_;
      float              targetCenter_;
      float              targetWidth_;

    public:
      UndoRedoCommand(const RadiographyWindowingTracker& tracker) :
        scene_(tracker.scene_),
        sourceCenter_(tracker.sourceCenter_),
        sourceWidth_(tracker.sourceWidth_)
      {
        scene_.GetWindowingWithDefault(targetCenter_, targetWidth_);
      }

      virtual void Undo() const
      {
        scene_.SetWindowing(sourceCenter_, sourceWidth_);
      }
      
      virtual void Redo() const
      {
        scene_.SetWindowing(targetCenter_, targetWidth_);
      }
    };


  public:
    RadiographyWindowingTracker(UndoRedoStack& undoRedoStack,
                                RadiographyScene& scene,
                                int x,
                                int y,
                                Action leftAction,
                                Action rightAction,
                                Action upAction,
                                Action downAction) :
      undoRedoStack_(undoRedoStack),
      scene_(scene),
      clickX_(x),
      clickY_(y),
      leftAction_(leftAction),
      rightAction_(rightAction),
      upAction_(upAction),
      downAction_(downAction)
    {
      scene_.GetWindowingWithDefault(sourceCenter_, sourceWidth_);

      float minValue, maxValue;
      scene.GetRange(minValue, maxValue);

      assert(minValue <= maxValue);

      float tmp;
      
      float delta = (maxValue - minValue);
      if (delta <= 1)
      {
        tmp = 0;
      }
      else
      {
        // NB: Visual Studio 2008 does not provide "log2f()", so we
        // implement it by ourselves
        tmp = logf(delta) / logf(2.0f);
      }

      strength_ = tmp - 7;
      if (strength_ < 1)
      {
        strength_ = 1;
      }
    }

    virtual bool HasRender() const
    {
      return false;
    }

    virtual void Render(CairoContext& context,
                        double zoom)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    virtual void MouseUp()
    {
      undoRedoStack_.Add(new UndoRedoCommand(*this));
    }


    virtual void MouseMove(int displayX,
                           int displayY,
                           double sceneX,
                           double sceneY)
    {
      // https://bitbucket.org/osimis/osimis-webviewer-plugin/src/master/frontend/src/app/viewport/image-plugins/windowing-viewport-tool.class.js

      static const float SCALE = 1.0;
      
      int deltaCenter = 0;
      int deltaWidth = 0;

      ComputeAxisEffect(deltaCenter, deltaWidth, displayX - clickX_, leftAction_, rightAction_);
      ComputeAxisEffect(deltaCenter, deltaWidth, displayY - clickY_, upAction_, downAction_);

      float newCenter = sourceCenter_ + (deltaCenter / SCALE * strength_);
      float newWidth = sourceWidth_ + (deltaWidth / SCALE * strength_);
      scene_.SetWindowing(newCenter, newWidth);
    }
  };



  
  namespace Samples
  {
    class RadiographyEditorInteractor :
      public IWorldSceneInteractor,
      public IObserver
    {
    private:
      enum Tool
      {
        Tool_Move,
        Tool_Rotate,
        Tool_Crop,
        Tool_Resize,
        Tool_Windowing
      };
        

      UndoRedoStack      undoRedoStack_;
      Tool               tool_;


      static double GetHandleSize()
      {
        return 10.0;
      }
    
         
    public:
      RadiographyEditorInteractor(MessageBroker& broker) :
        IObserver(broker),
        tool_(Tool_Move)
      {
      }
    
      virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& worldWidget,
                                                          const ViewportGeometry& view,
                                                          MouseButton button,
                                                          KeyboardModifiers modifiers,
                                                          int viewportX,
                                                          int viewportY,
                                                          double x,
                                                          double y,
                                                          IStatusBar* statusBar)
      {
        RadiographyWidget& widget = dynamic_cast<RadiographyWidget&>(worldWidget);

        if (button == MouseButton_Left)
        {
          size_t selected;
        
          if (tool_ == Tool_Windowing)
          {
            return new RadiographyWindowingTracker(
              undoRedoStack_, widget.GetScene(),
              viewportX, viewportY,
              RadiographyWindowingTracker::Action_DecreaseWidth,
              RadiographyWindowingTracker::Action_IncreaseWidth,
              RadiographyWindowingTracker::Action_DecreaseCenter,
              RadiographyWindowingTracker::Action_IncreaseCenter);
          }
          else if (!widget.LookupSelectedLayer(selected))
          {
            // No layer is currently selected
            size_t layer;
            if (widget.GetScene().LookupLayer(layer, x, y))
            {
              widget.Select(layer);
            }

            return NULL;
          }
          else if (tool_ == Tool_Crop ||
                   tool_ == Tool_Resize)
          {
            RadiographyScene::LayerAccessor accessor(widget.GetScene(), selected);
            
            Corner corner;
            if (accessor.GetLayer().LookupCorner(corner, x, y, view.GetZoom(), GetHandleSize()))
            {
              switch (tool_)
              {
                case Tool_Crop:
                  return new RadiographyLayerCropTracker
                    (undoRedoStack_, widget.GetScene(), view, selected, x, y, corner);

                case Tool_Resize:
                  return new RadiographyLayerResizeTracker
                    (undoRedoStack_, widget.GetScene(), selected, x, y, corner,
                     (modifiers & KeyboardModifiers_Shift));

                default:
                  throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
              }
            }
            else
            {
              size_t layer;
            
              if (widget.GetScene().LookupLayer(layer, x, y))
              {
                widget.Select(layer);
              }
              else
              {
                widget.Unselect();
              }
            
              return NULL;
            }
          }
          else
          {
            size_t layer;

            if (widget.GetScene().LookupLayer(layer, x, y))
            {
              if (layer == selected)
              {
                switch (tool_)
                {
                  case Tool_Move:
                    return new RadiographyLayerMoveTracker
                      (undoRedoStack_, widget.GetScene(), layer, x, y,
                       (modifiers & KeyboardModifiers_Shift));

                  case Tool_Rotate:
                    return new RadiographyLayerRotateTracker
                      (undoRedoStack_, widget.GetScene(), view, layer, x, y,
                       (modifiers & KeyboardModifiers_Shift));
                
                  default:
                    break;
                }

                return NULL;
              }
              else
              {
                widget.Select(layer);
                return NULL;
              }
            }
            else
            {
              widget.Unselect();
              return NULL;
            }
          }
        }
        else
        {
          return NULL;
        }
      }

      virtual void MouseOver(CairoContext& context,
                             WorldSceneWidget& worldWidget,
                             const ViewportGeometry& view,
                             double x,
                             double y,
                             IStatusBar* statusBar)
      {
        RadiographyWidget& widget = dynamic_cast<RadiographyWidget&>(worldWidget);

#if 0
        if (statusBar != NULL)
        {
          char buf[64];
          sprintf(buf, "X = %.02f Y = %.02f (in cm)", x / 10.0, y / 10.0);
          statusBar->SetMessage(buf);
        }
#endif

        size_t selected;

        if (widget.LookupSelectedLayer(selected) &&
            (tool_ == Tool_Crop ||
             tool_ == Tool_Resize))
        {
          RadiographyScene::LayerAccessor accessor(widget.GetScene(), selected);
        
          Corner corner;
          if (accessor.GetLayer().LookupCorner(corner, x, y, view.GetZoom(), GetHandleSize()))
          {
            accessor.GetLayer().GetCorner(x, y, corner);
          
            double z = 1.0 / view.GetZoom();
          
            context.SetSourceColor(255, 0, 0);
            cairo_t* cr = context.GetObject();
            cairo_set_line_width(cr, 2.0 * z);
            cairo_move_to(cr, x - GetHandleSize() * z, y - GetHandleSize() * z);
            cairo_line_to(cr, x + GetHandleSize() * z, y - GetHandleSize() * z);
            cairo_line_to(cr, x + GetHandleSize() * z, y + GetHandleSize() * z);
            cairo_line_to(cr, x - GetHandleSize() * z, y + GetHandleSize() * z);
            cairo_line_to(cr, x - GetHandleSize() * z, y - GetHandleSize() * z);
            cairo_stroke(cr);
          }
        }
      }

      virtual void MouseWheel(WorldSceneWidget& widget,
                              MouseWheelDirection direction,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
      }

      virtual void KeyPressed(WorldSceneWidget& worldWidget,
                              KeyboardKeys key,
                              char keyChar,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
        RadiographyWidget& widget = dynamic_cast<RadiographyWidget&>(worldWidget);

        switch (keyChar)
        {
          case 'a':
            widget.FitContent();
            break;

          case 'c':
            tool_ = Tool_Crop;
            break;

          case 'e':
          {
            Orthanc::DicomMap tags;

            // Minimal set of tags to generate a valid CR image
            tags.SetValue(Orthanc::DICOM_TAG_ACCESSION_NUMBER, "NOPE", false);
            tags.SetValue(Orthanc::DICOM_TAG_BODY_PART_EXAMINED, "PELVIS", false);
            tags.SetValue(Orthanc::DICOM_TAG_INSTANCE_NUMBER, "1", false);
            //tags.SetValue(Orthanc::DICOM_TAG_LATERALITY, "", false);
            tags.SetValue(Orthanc::DICOM_TAG_MANUFACTURER, "OSIMIS", false);
            tags.SetValue(Orthanc::DICOM_TAG_MODALITY, "CR", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_BIRTH_DATE, "20000101", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_ID, "hello", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_NAME, "HELLO^WORLD", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_ORIENTATION, "", false);
            tags.SetValue(Orthanc::DICOM_TAG_PATIENT_SEX, "M", false);
            tags.SetValue(Orthanc::DICOM_TAG_REFERRING_PHYSICIAN_NAME, "HOUSE^MD", false);
            tags.SetValue(Orthanc::DICOM_TAG_SERIES_NUMBER, "1", false);
            tags.SetValue(Orthanc::DICOM_TAG_SOP_CLASS_UID, "1.2.840.10008.5.1.4.1.1.1", false);
            tags.SetValue(Orthanc::DICOM_TAG_STUDY_ID, "STUDY", false);
            tags.SetValue(Orthanc::DICOM_TAG_VIEW_POSITION, "", false);

            widget.GetScene().ExportDicom(tags, 0.1, 0.1, widget.IsInverted(),
                                          widget.GetInterpolation(), EXPORT_USING_PAM);
            break;
          }

          case 'i':
            widget.SwitchInvert();
            break;
        
          case 'm':
            tool_ = Tool_Move;
            break;

          case 'n':
          {
            switch (widget.GetInterpolation())
            {
              case ImageInterpolation_Nearest:
                LOG(INFO) << "Switching to bilinear interpolation";
                widget.SetInterpolation(ImageInterpolation_Bilinear);
                break;
              
              case ImageInterpolation_Bilinear:
                LOG(INFO) << "Switching to nearest neighbor interpolation";
                widget.SetInterpolation(ImageInterpolation_Nearest);
                break;

              default:
                throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }
          
            break;
          }
        
          case 'r':
            tool_ = Tool_Rotate;
            break;

          case 's':
            tool_ = Tool_Resize;
            break;

          case 'w':
            tool_ = Tool_Windowing;
            break;

          case 'y':
            if (modifiers & KeyboardModifiers_Control)
            {
              undoRedoStack_.Redo();
              widget.NotifyContentChanged();
            }
            break;

          case 'z':
            if (modifiers & KeyboardModifiers_Control)
            {
              undoRedoStack_.Undo();
              widget.NotifyContentChanged();
            }
            break;
        
          default:
            break;
        }
      }
    };

  
  
    class SingleFrameEditorApplication :
      public SampleSingleCanvasApplicationBase,
      public IObserver
    {
    private:
      std::auto_ptr<OrthancApiClient>  orthancApiClient_;
      std::auto_ptr<RadiographyScene>  scene_;
      RadiographyEditorInteractor      interactor_;

    public:
      SingleFrameEditorApplication(MessageBroker& broker) :
        IObserver(broker),
        interactor_(broker)
      {
      }

      virtual ~SingleFrameEditorApplication()
      {
        LOG(WARNING) << "Destroying the application";
      }
      
      virtual void DeclareStartupOptions(boost::program_options::options_description& options)
      {
        boost::program_options::options_description generic("Sample options");
        generic.add_options()
          ("instance", boost::program_options::value<std::string>(),
           "Orthanc ID of the instance")
          ("frame", boost::program_options::value<unsigned int>()->default_value(0),
           "Number of the frame, for multi-frame DICOM instances")
          ;

        options.add(generic);
      }

      virtual void Initialize(StoneApplicationContext* context,
                              IStatusBar& statusBar,
                              const boost::program_options::variables_map& parameters)
      {
        using namespace OrthancStone;

        context_ = context;

        statusBar.SetMessage("Use the key \"a\" to reinitialize the layout");
        statusBar.SetMessage("Use the key \"c\" to crop");
        statusBar.SetMessage("Use the key \"e\" to export DICOM to the Orthanc server");
        statusBar.SetMessage("Use the key \"f\" to switch full screen");
        statusBar.SetMessage("Use the key \"i\" to invert contrast");
        statusBar.SetMessage("Use the key \"m\" to move objects");
        statusBar.SetMessage("Use the key \"n\" to switch between nearest neighbor and bilinear interpolation");
        statusBar.SetMessage("Use the key \"r\" to rotate objects");
        statusBar.SetMessage("Use the key \"s\" to resize objects (not applicable to DICOM layers)");
        statusBar.SetMessage("Use the key \"w\" to change windowing");
        
        statusBar.SetMessage("Use the key \"ctrl-z\" to undo action");
        statusBar.SetMessage("Use the key \"ctrl-y\" to redo action");

        if (parameters.count("instance") != 1)
        {
          LOG(ERROR) << "The instance ID is missing";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
        }

        std::string instance = parameters["instance"].as<std::string>();
        int frame = parameters["frame"].as<unsigned int>();

        orthancApiClient_.reset(new OrthancApiClient(GetBroker(), context_->GetWebService()));

        Orthanc::FontRegistry fonts;
        fonts.AddFromResource(Orthanc::EmbeddedResources::FONT_UBUNTU_MONO_BOLD_16);
        
        scene_.reset(new RadiographyScene(GetBroker(), *orthancApiClient_));
        scene_->LoadDicomFrame(instance, frame, false); //.SetPan(200, 0);
        //scene_->LoadDicomFrame("61f3143e-96f34791-ad6bbb8d-62559e75-45943e1b", 0, false);

        {
          RadiographyLayer& layer = scene_->LoadText(fonts.GetFont(0), "Hello\nworld");
          layer.SetResizeable(true);
        }
        
        {
          RadiographyLayer& layer = scene_->LoadTestBlock(100, 50);
          layer.SetResizeable(true);
          layer.SetPan(0, 200);
        }
        
        
        mainWidget_ = new RadiographyWidget(GetBroker(), *scene_, "main-widget");
        mainWidget_->SetTransmitMouseOver(true);
        mainWidget_->SetInteractor(interactor_);

        //scene_->SetWindowing(128, 256);
      }
    };
  }
}
