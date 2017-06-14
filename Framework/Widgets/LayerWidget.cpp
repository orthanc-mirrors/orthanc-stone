/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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


#include "LayerWidget.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../Layers/SliceOutlineRenderer.h"

static const double THIN_SLICE_THICKNESS = 100.0 * std::numeric_limits<double>::epsilon();

namespace OrthancStone
{
  class LayerWidget::Scene : public boost::noncopyable
  {
  private:
    CoordinateSystem3D            slice_;
    size_t                        countMissing_;
    std::vector<ILayerRenderer*>  renderers_;

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
      
  public:
    Scene(const CoordinateSystem3D& slice,
          size_t countLayers) :
      slice_(slice),
      countMissing_(countLayers),
      renderers_(countLayers, NULL)
    {
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

    const CoordinateSystem3D& GetSlice() const
    {
      return slice_;
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
      return countMissing_;
    }

    bool RenderScene(CairoContext& context,
                     const ViewportGeometry& view,
                     const CoordinateSystem3D& viewportSlice)
    {
      bool fullQuality = true;
      cairo_t *cr = context.GetObject();

      for (size_t i = 0; i < renderers_.size(); i++)
      {
        if (renderers_[i] != NULL)
        {
          const CoordinateSystem3D& frameSlice = renderers_[i]->GetLayerSlice();
          
          double x0, y0, x1, y1, x2, y2;
          viewportSlice.ProjectPoint(x0, y0, frameSlice.GetOrigin());
          viewportSlice.ProjectPoint(x1, y1, frameSlice.GetOrigin() + frameSlice.GetAxisX());
          viewportSlice.ProjectPoint(x2, y2, frameSlice.GetOrigin() + frameSlice.GetAxisY());

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
        // TODO Drawing circles makes WebAssembly crash!
        cairo_arc(cr, 0, 0, 5.0 / view.GetZoom(), 0, 2 * M_PI);
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
  };

  
  bool LayerWidget::LookupLayer(size_t& index /* out */,
                                const ILayerSource& layer) const
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
    

  void LayerWidget::GetLayerExtent(Extent& extent,
                                   ILayerSource& source) const
  {
    extent.Reset();
    
    std::vector<Vector> points;
    if (source.GetExtent(points, slice_))
    {
      for (size_t i = 0; i < points.size(); i++)
      {
        double x, y;
        slice_.ProjectPoint(x, y, points[i]);
        extent.AddPoint(x, y);
      }
    }
  }

        
  Extent LayerWidget::GetSceneExtent()
  {
    Extent sceneExtent;

    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      Extent layerExtent;
      GetLayerExtent(layerExtent, *layers_[i]);

      sceneExtent.Union(layerExtent);
    }

    return sceneExtent;
  }

  
  bool LayerWidget::RenderScene(CairoContext& context,
                                const ViewportGeometry& view)
  {
    if (currentScene_.get() != NULL)
    {
      return currentScene_->RenderScene(context, view, slice_);
    }
    else
    {
      return true;
    }
  }

  
  void LayerWidget::ResetPendingScene()
  {
    pendingScene_.reset(new Scene(slice_, layers_.size()));
  }
  

  void LayerWidget::UpdateLayer(size_t index,
                                ILayerRenderer* renderer,
                                const Slice& slice)
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
        slice.ContainsPlane(currentScene_->GetSlice()))
    {
      currentScene_->SetLayer(index, tmp.release());
      NotifyChange();
    }
    else if (pendingScene_.get() != NULL &&
             slice.ContainsPlane(pendingScene_->GetSlice()))
    {
      pendingScene_->SetLayer(index, tmp.release());

      if (currentScene_.get() == NULL ||
          !currentScene_->IsComplete() ||
          pendingScene_->IsComplete())
      {
        currentScene_ = pendingScene_;
        NotifyChange();
      }
    }
  }

  
  LayerWidget::LayerWidget() :
    started_(false)
  {
    SetBackgroundCleared(true);
  }
  
  
  LayerWidget::~LayerWidget()
  {
    for (size_t i = 0; i < layers_.size(); i++)
    {
      delete layers_[i];
    }
  }
  

  size_t LayerWidget::AddLayer(ILayerSource* layer)  // Takes ownership
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
    layer->Register(*this);

    ResetChangedLayers();

    return index;
  }

  
  void LayerWidget::SetLayerStyle(size_t layer,
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

    NotifyChange();
  }
  

  void LayerWidget::SetSlice(const CoordinateSystem3D& slice)
  {
    LOG(INFO) << "Setting slice origin: (" << slice.GetOrigin()[0]
              << "," << slice.GetOrigin()[1]
              << "," << slice.GetOrigin()[2] << ")";
    
    Slice displayedSlice(slice_, THIN_SLICE_THICKNESS);

    //if (!displayedSlice.ContainsPlane(slice))
    {
      if (currentScene_.get() == NULL ||
          (pendingScene_.get() != NULL &&
           pendingScene_->IsComplete()))
      {
        currentScene_ = pendingScene_;
      }

      slice_ = slice;
      ResetPendingScene();

      InvalidateAllLayers();   // TODO Removing this line avoid loading twice the image in WASM
    }
  }


  void LayerWidget::NotifyGeometryReady(const ILayerSource& source)
  {
    size_t i;
    if (LookupLayer(i, source))
    {
      LOG(INFO) << "Geometry ready for layer " << i;

      changedLayers_[i] = true;
      //layers_[i]->ScheduleLayerCreation(slice_);
    }
  }
  

  void LayerWidget::NotifyGeometryError(const ILayerSource& source)
  {
    LOG(ERROR) << "Cannot get geometry";
  }
  

  void LayerWidget::InvalidateAllLayers()
  {
    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      changedLayers_[i] = true;
      
      //layers_[i]->ScheduleLayerCreation(slice_);
    }
  }


  void LayerWidget::InvalidateLayer(size_t layer)
  {
    if (layer >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    assert(layers_[layer] != NULL);
    changedLayers_[layer] = true;

    //layers_[layer]->ScheduleLayerCreation(slice_);
  }


  void LayerWidget::NotifyContentChange(const ILayerSource& source)
  {
    size_t index;
    if (LookupLayer(index, source))
    {
      InvalidateLayer(index);
    }
  }
  

  void LayerWidget::NotifySliceChange(const ILayerSource& source,
                                      const Slice& slice)
  {
    if (slice.ContainsPlane(slice_))
    {
      size_t index;
      if (LookupLayer(index, source))
      {
        InvalidateLayer(index);
      }
    }
  }
  
  
  void LayerWidget::NotifyLayerReady(std::auto_ptr<ILayerRenderer>& renderer,
                                     const ILayerSource& source,
                                     const Slice& slice,
                                     bool isError)
  {
    size_t index;
    if (slice.IsValid() &&
        LookupLayer(index, source) &&
        slice.ContainsPlane(slice_))  // Whether the slice comes from an older request
    {
      if (isError)
      {
        LOG(ERROR) << "Using error renderer on layer " << index;
      }
      else
      {
        LOG(INFO) << "Renderer ready for layer " << index;
      }
      
      if (renderer.get() != NULL)
      {
        UpdateLayer(index, renderer.release(), slice);
      }
      else if (isError)
      {
        UpdateLayer(index, new SliceOutlineRenderer(slice), slice);
      }
    }
  }


  void LayerWidget::ResetChangedLayers()
  {
    changedLayers_.resize(layers_.size());

    for (size_t i = 0; i < changedLayers_.size(); i++)
    {
      changedLayers_[i] = false;
    }
  }


  void LayerWidget::UpdateContent()
  {
    assert(changedLayers_.size() <= layers_.size());
    
    for (size_t i = 0; i < changedLayers_.size(); i++)
    {
      if (changedLayers_[i])
      {
        layers_[i]->ScheduleLayerCreation(slice_);
      }
    }
    
    ResetChangedLayers();
  }
}
