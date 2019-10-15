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


#include "SliceViewerWidget.h"

#include "../Layers/SliceOutlineRenderer.h"
#include "../../Toolbox/GeometryToolbox.h"
#include "../Layers/FrameRenderer.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/math/constants/constants.hpp>


static const double THIN_SLICE_THICKNESS = 100.0 * std::numeric_limits<double>::epsilon();

namespace Deprecated
{
  class SliceViewerWidget::Scene : public boost::noncopyable
  {
  private:
    OrthancStone::CoordinateSystem3D            plane_;
    double                        thickness_;
    size_t                        countMissing_;
    std::vector<ILayerRenderer*>  renderers_;

  public:
    void DeleteLayer(size_t index)
    {
      if (index >= renderers_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      assert(countMissing_ <= renderers_.size());

      if (renderers_[index] != NULL)
      {
        assert(countMissing_ < renderers_.size());
        delete renderers_[index];
        renderers_[index] = NULL;
        countMissing_++;
      }
    }

    Scene(const OrthancStone::CoordinateSystem3D& plane,
          double thickness,
          size_t countLayers) :
      plane_(plane),
      thickness_(thickness),
      countMissing_(countLayers),
      renderers_(countLayers, NULL)
    {
      if (thickness <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    ~Scene()
    {
      for (size_t i = 0; i < renderers_.size(); i++)
      {
        DeleteLayer(i);
      }
    }

    void SetLayer(size_t index,
                  ILayerRenderer* renderer)  // Takes ownership
    {
      if (renderer == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      DeleteLayer(index);

      renderers_[index] = renderer;
      countMissing_--;
    }

    const OrthancStone::CoordinateSystem3D& GetPlane() const
    {
      return plane_;
    }

    bool HasRenderer(size_t index)
    {
      return renderers_[index] != NULL;
    }

    bool IsComplete() const
    {
      return countMissing_ == 0;
    }

    unsigned int GetCountMissing() const
    {
      return static_cast<unsigned int>(countMissing_);
    }

    bool RenderScene(OrthancStone::CairoContext& context,
                     const ViewportGeometry& view,
                     const OrthancStone::CoordinateSystem3D& viewportPlane)
    {
      bool fullQuality = true;
      cairo_t *cr = context.GetObject();

      for (size_t i = 0; i < renderers_.size(); i++)
      {
        if (renderers_[i] != NULL)
        {
          const OrthancStone::CoordinateSystem3D& framePlane = renderers_[i]->GetLayerPlane();
          
          double x0, y0, x1, y1, x2, y2;
          viewportPlane.ProjectPoint(x0, y0, framePlane.GetOrigin());
          viewportPlane.ProjectPoint(x1, y1, framePlane.GetOrigin() + framePlane.GetAxisX());
          viewportPlane.ProjectPoint(x2, y2, framePlane.GetOrigin() + framePlane.GetAxisY());

          /**
           * Now we solve the system of linear equations Ax + b = x', given:
           *   A [0 ; 0] + b = [x0 ; y0]
           *   A [1 ; 0] + b = [x1 ; y1]
           *   A [0 ; 1] + b = [x2 ; y2]
           * <=>
           *   b = [x0 ; y0]
           *   A [1 ; 0] = [x1 ; y1] - b = [x1 - x0 ; y1 - y0]
           *   A [0 ; 1] = [x2 ; y2] - b = [x2 - x0 ; y2 - y0]
           * <=>
           *   b = [x0 ; y0]
           *   [a11 ; a21] = [x1 - x0 ; y1 - y0]
           *   [a12 ; a22] = [x2 - x0 ; y2 - y0]
           **/

          cairo_matrix_t transform;
          cairo_matrix_init(&transform, x1 - x0, y1 - y0, x2 - x0, y2 - y0, x0, y0);

          cairo_save(cr);
          cairo_transform(cr, &transform);
          
          if (!renderers_[i]->RenderLayer(context, view))
          {
            cairo_restore(cr);
            return false;
          }

          cairo_restore(cr);
        }

        if (renderers_[i] != NULL &&
            !renderers_[i]->IsFullQuality())
        {
          fullQuality = false;
        }
      }

      if (!fullQuality)
      {
        double x, y;
        view.MapDisplayToScene(x, y, static_cast<double>(view.GetDisplayWidth()) / 2.0, 10);

        cairo_translate(cr, x, y);

#if 1
        double s = 5.0 / view.GetZoom();
        cairo_rectangle(cr, -s, -s, 2.0 * s, 2.0 * s);
#else
        // TODO Drawing filled circles makes WebAssembly crash!
        cairo_arc(cr, 0, 0, 5.0 / view.GetZoom(), 0, 2.0 * boost::math::constants::pi<double>());
#endif
        
        cairo_set_line_width(cr, 2.0 / view.GetZoom());
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_stroke_preserve(cr);
        cairo_set_source_rgb(cr, 1, 0, 0);
        cairo_fill(cr);
      }

      return true;
    }

    void SetLayerStyle(size_t index,
                       const RenderStyle& style)
    {
      if (renderers_[index] != NULL)
      {
        renderers_[index]->SetLayerStyle(style);
      }
    }

    bool ContainsPlane(const OrthancStone::CoordinateSystem3D& plane) const
    {
      bool isOpposite;
      if (!OrthancStone::GeometryToolbox::IsParallelOrOpposite(isOpposite,
                                                               plane.GetNormal(),
                                                               plane_.GetNormal()))
      {
        return false;
      }
      else
      {
        double z = (plane_.ProjectAlongNormal(plane.GetOrigin()) -
                    plane_.ProjectAlongNormal(plane_.GetOrigin()));

        if (z < 0)
        {
          z = -z;
        }

        return z <= thickness_;
      }
    }

    double GetThickness() const
    {
      return thickness_;
    }
  };

  
  bool SliceViewerWidget::LookupLayer(size_t& index /* out */,
                                      const IVolumeSlicer& layer) const
  {
    LayersIndex::const_iterator found = layersIndex_.find(&layer);

    if (found == layersIndex_.end())
    {
      return false;
    }
    else
    {
      index = found->second;
      assert(index < layers_.size() &&
             layers_[index] == &layer);
      return true;
    }
  }


  void SliceViewerWidget::GetLayerExtent(OrthancStone::Extent2D& extent,
                                         IVolumeSlicer& source) const
  {
    extent.Reset();

    std::vector<OrthancStone::Vector> points;
    if (source.GetExtent(points, plane_))
    {
      for (size_t i = 0; i < points.size(); i++)
      {
        double x, y;
        plane_.ProjectPoint(x, y, points[i]);
        extent.AddPoint(x, y);
      }
    }
  }


  OrthancStone::Extent2D SliceViewerWidget::GetSceneExtent()
  {
    OrthancStone::Extent2D sceneExtent;

    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      OrthancStone::Extent2D layerExtent;
      GetLayerExtent(layerExtent, *layers_[i]);

      sceneExtent.Union(layerExtent);
    }

    return sceneExtent;
  }

  
  bool SliceViewerWidget::RenderScene(OrthancStone::CairoContext& context,
                                      const ViewportGeometry& view)
  {
    if (currentScene_.get() != NULL)
    {
      return currentScene_->RenderScene(context, view, plane_);
    }
    else
    {
      return true;
    }
  }

  
  void SliceViewerWidget::ResetPendingScene()
  {
    double thickness;
    if (pendingScene_.get() == NULL)
    {
      thickness = 1.0;
    }
    else
    {
      thickness = pendingScene_->GetThickness();
    }
    
    pendingScene_.reset(new Scene(plane_, thickness, layers_.size()));
  }
  

  void SliceViewerWidget::UpdateLayer(size_t index,
                                      ILayerRenderer* renderer,
                                      const OrthancStone::CoordinateSystem3D& plane)
  {
    LOG(INFO) << "Updating layer " << index;
    
    std::auto_ptr<ILayerRenderer> tmp(renderer);

    if (renderer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (index >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    assert(layers_.size() == styles_.size());
    renderer->SetLayerStyle(styles_[index]);

    if (currentScene_.get() != NULL &&
        currentScene_->ContainsPlane(plane))
    {
      currentScene_->SetLayer(index, tmp.release());
      NotifyContentChanged();
    }
    else if (pendingScene_.get() != NULL &&
             pendingScene_->ContainsPlane(plane))
    {
      pendingScene_->SetLayer(index, tmp.release());

      if (currentScene_.get() == NULL ||
          !currentScene_->IsComplete() ||
          pendingScene_->IsComplete())
      {
        currentScene_ = pendingScene_;
        NotifyContentChanged();
      }
    }
  }

  
  SliceViewerWidget::SliceViewerWidget(const std::string& name) :
    WorldSceneWidget(name),
    started_(false)
  {
    SetBackgroundCleared(true);
  }
  
  
  SliceViewerWidget::~SliceViewerWidget()
  {
    for (size_t i = 0; i < layers_.size(); i++)
    {
      delete layers_[i];
    }
  }
  
  void SliceViewerWidget::ObserveLayer(IVolumeSlicer& layer)
  {
    // currently ignoring errors of type IVolumeSlicer::GeometryErrorMessage

    Register<IVolumeSlicer::GeometryReadyMessage>(layer, &SliceViewerWidget::OnGeometryReady);
    Register<IVolumeSlicer::SliceContentChangedMessage>(layer, &SliceViewerWidget::OnSliceChanged);
    Register<IVolumeSlicer::ContentChangedMessage>(layer, &SliceViewerWidget::OnContentChanged);
    Register<IVolumeSlicer::LayerReadyMessage>(layer, &SliceViewerWidget::OnLayerReady);
    Register<IVolumeSlicer::LayerErrorMessage>(layer, &SliceViewerWidget::OnLayerError);
  }


  size_t SliceViewerWidget::AddLayer(IVolumeSlicer* layer)  // Takes ownership
  {
    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    size_t index = layers_.size();
    layers_.push_back(layer);
    styles_.push_back(RenderStyle());
    layersIndex_[layer] = index;

    ResetPendingScene();

    ObserveLayer(*layer);

    ResetChangedLayers();

    return index;
  }


  void SliceViewerWidget::ReplaceLayer(size_t index, IVolumeSlicer* layer)  // Takes ownership
  {
    if (layer == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (index >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    delete layers_[index];
    layers_[index] = layer;
    layersIndex_[layer] = index;

    ResetPendingScene();

    ObserveLayer(*layer);

    InvalidateLayer(index);
  }


  void SliceViewerWidget::RemoveLayer(size_t index)
  {
    if (index >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    IVolumeSlicer* previousLayer = layers_[index];
    layersIndex_.erase(layersIndex_.find(previousLayer));
    layers_.erase(layers_.begin() + index);
    changedLayers_.erase(changedLayers_.begin() + index);
    styles_.erase(styles_.begin() + index);

    delete layers_[index];

    currentScene_->DeleteLayer(index);
    ResetPendingScene();

    NotifyContentChanged();
  }


  const RenderStyle& SliceViewerWidget::GetLayerStyle(size_t layer) const
  {
    if (layer >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    assert(layers_.size() == styles_.size());
    return styles_[layer];
  }
  

  void SliceViewerWidget::SetLayerStyle(size_t layer,
                                        const RenderStyle& style)
  {
    if (layer >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    assert(layers_.size() == styles_.size());
    styles_[layer] = style;

    if (currentScene_.get() != NULL)
    {
      currentScene_->SetLayerStyle(layer, style);
    }

    if (pendingScene_.get() != NULL)
    {
      pendingScene_->SetLayerStyle(layer, style);
    }

    NotifyContentChanged();
  }
  

  void SliceViewerWidget::SetSlice(const OrthancStone::CoordinateSystem3D& plane)
  {
    LOG(INFO) << "Setting slice origin: (" << plane.GetOrigin()[0]
              << "," << plane.GetOrigin()[1]
              << "," << plane.GetOrigin()[2] << ")";
    
    Deprecated::Slice displayedSlice(plane_, THIN_SLICE_THICKNESS);

    //if (!displayedSlice.ContainsPlane(slice))
    {
      if (currentScene_.get() == NULL ||
          (pendingScene_.get() != NULL &&
           pendingScene_->IsComplete()))
      {
        currentScene_ = pendingScene_;
      }

      plane_ = plane;
      ResetPendingScene();

      InvalidateAllLayers();   // TODO Removing this line avoid loading twice the image in WASM
    }

    BroadcastMessage(DisplayedSliceMessage(*this, displayedSlice));
  }


  void SliceViewerWidget::OnGeometryReady(const IVolumeSlicer::GeometryReadyMessage& message)
  {
    size_t i;
    if (LookupLayer(i, message.GetOrigin()))
    {
      LOG(INFO) << ": Geometry ready for layer " << i << " in " << GetName();

      changedLayers_[i] = true;
      //layers_[i]->ScheduleLayerCreation(plane_);
    }
    BroadcastMessage(GeometryChangedMessage(*this));
  }
  

  void SliceViewerWidget::InvalidateAllLayers()
  {
    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      changedLayers_[i] = true;
      
      //layers_[i]->ScheduleLayerCreation(plane_);
    }
  }


  void SliceViewerWidget::InvalidateLayer(size_t layer)
  {
    if (layer >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    assert(layers_[layer] != NULL);
    changedLayers_[layer] = true;

    //layers_[layer]->ScheduleLayerCreation(plane_);
  }


  void SliceViewerWidget::OnContentChanged(const IVolumeSlicer::ContentChangedMessage& message)
  {
    size_t index;
    if (LookupLayer(index, message.GetOrigin()))
    {
      InvalidateLayer(index);
    }
    
    BroadcastMessage(SliceViewerWidget::ContentChangedMessage(*this));
  }
  

  void SliceViewerWidget::OnSliceChanged(const IVolumeSlicer::SliceContentChangedMessage& message)
  {
    if (message.GetSlice().ContainsPlane(plane_))
    {
      size_t index;
      if (LookupLayer(index, message.GetOrigin()))
      {
        InvalidateLayer(index);
      }
    }
    
    BroadcastMessage(SliceViewerWidget::ContentChangedMessage(*this));
  }
  
  
  void SliceViewerWidget::OnLayerReady(const IVolumeSlicer::LayerReadyMessage& message)
  {
    size_t index;
    if (LookupLayer(index, message.GetOrigin()))
    {
      LOG(INFO) << "Renderer ready for layer " << index;
      UpdateLayer(index, message.CreateRenderer(), message.GetSlice());
    }
    
    BroadcastMessage(SliceViewerWidget::ContentChangedMessage(*this));
  }


  void SliceViewerWidget::OnLayerError(const IVolumeSlicer::LayerErrorMessage& message)
  {
    size_t index;
    if (LookupLayer(index, message.GetOrigin()))
    {
      LOG(ERROR) << "Using error renderer on layer " << index;

      // TODO
      //UpdateLayer(index, new SliceOutlineRenderer(slice), slice);

      BroadcastMessage(SliceViewerWidget::ContentChangedMessage(*this));
    }
  }


  void SliceViewerWidget::ResetChangedLayers()
  {
    changedLayers_.resize(layers_.size());

    for (size_t i = 0; i < changedLayers_.size(); i++)
    {
      changedLayers_[i] = false;
    }
  }


  void SliceViewerWidget::DoAnimation()
  {
    assert(changedLayers_.size() <= layers_.size());
    
    for (size_t i = 0; i < changedLayers_.size(); i++)
    {
      if (changedLayers_[i])
      {
        layers_[i]->ScheduleLayerCreation(plane_);
      }
    }
    
    ResetChangedLayers();
  }
}
