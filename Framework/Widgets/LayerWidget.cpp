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

namespace OrthancStone
{
  class LayerWidget::Scene : public boost::noncopyable
  {
  private:
    SliceGeometry                 slice_;
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
    Scene(const SliceGeometry& slice,
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

    const SliceGeometry& GetSlice() const
    {
      return slice_;
    }

    bool IsComplete() const
    {
      return countMissing_ == 0;
    }

    bool RenderScene(CairoContext& context,
                     const ViewportGeometry& view)
    {
      bool fullQuality = true;

      for (size_t i = 0; i < renderers_.size(); i++)
      {
        if (renderers_[i] != NULL &&
            !renderers_[i]->RenderLayer(context, view, slice_))
        {
          return false;
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

        cairo_t *cr = context.GetObject();
        cairo_translate(cr, x, y);
        cairo_arc(cr, 0, 0, 5.0 / view.GetZoom(), 0, 2 * M_PI);
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
                                ILayerSource& layer) const
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
    
        
  void LayerWidget::GetSceneExtent(double& x1,
                                   double& y1,
                                   double& x2,
                                   double& y2)
  {
    bool first = true;

    for (size_t i = 0; i < layers_.size(); i++)
    {
      double ax, ay, bx, by;

      assert(layers_[i] != NULL);
      if (layers_[i]->GetExtent(ax, ay, bx, by, slice_))
      {
        if (ax > bx)
        {
          std::swap(ax, bx);
        }

        if (ay > by)
        {
          std::swap(ay, by);
        }

        LOG(INFO) << "Extent of layer " << i << ": (" << ax << "," << ay << ")->(" << bx << "," << by << ")";

        if (first)
        {
          x1 = ax;
          y1 = ay;
          x2 = bx;
          y2 = by;
          first = false;
        }
        else
        {
          x1 = std::min(x1, ax);
          y1 = std::min(y1, ay);
          x2 = std::max(x2, bx);
          y2 = std::max(y2, by);
        }
      }
    }

    if (first)
    {
      // Set a default extent of (-1,-1) -> (0,0)
      x1 = -1;
      y1 = -1;
      x2 = 1;
      y2 = 1;
    }

    // Ensure the extent is non-empty
    if (x1 >= x2)
    {
      double tmp = x1;
      x1 = tmp - 0.5;
      x2 = tmp + 0.5;
    }

    if (y1 >= y2)
    {
      double tmp = y1;
      y1 = tmp - 0.5;
      y2 = tmp + 0.5;
    }
  }

  
  bool LayerWidget::RenderScene(CairoContext& context,
                                const ViewportGeometry& view)
  {
    if (currentScene_.get() != NULL)
    {
      return currentScene_->RenderScene(context, view);
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
    layer->SetObserver(*this);

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
  

  void LayerWidget::SetSlice(const SliceGeometry& slice)
  {
    if (!slice_.IsSamePlane(slice, 100.0 * std::numeric_limits<double>::epsilon()))
    {
      if (currentScene_.get() == NULL ||
          (pendingScene_.get() != NULL &&
           pendingScene_->IsComplete()))
      {
        currentScene_ = pendingScene_;
      }
        
      slice_ = slice;
      ResetPendingScene();

      if (started_)
      {
        for (size_t i = 0; i < layers_.size(); i++)
        {
          assert(layers_[i] != NULL);
          layers_[i]->ScheduleLayerCreation(slice_);
        }
      }
    }
  }

  
  void LayerWidget::NotifyGeometryReady(ILayerSource& source)
  {
    size_t i;
    if (LookupLayer(i, source))
    {
      LOG(INFO) << "Geometry ready for layer " << i;
      SetDefaultView();
      layers_[i]->ScheduleLayerCreation(slice_);
    }
  }
  

  void LayerWidget::NotifyGeometryError(ILayerSource& source)
  {
    LOG(ERROR) << "Cannot get geometry";
  }
  

  void LayerWidget::NotifySourceChange(ILayerSource& source)
  {
    source.ScheduleLayerCreation(slice_);
  }
  

  void LayerWidget::NotifySliceChange(ILayerSource& source,
                                      const Slice& slice)
  {
    if (slice.ContainsPlane(slice_))
    {
      source.ScheduleLayerCreation(slice_);
    }
  }
  

  void LayerWidget::NotifyLayerReady(ILayerRenderer* renderer,
                                     ILayerSource& source,
                                     const Slice& slice)
  {
    std::auto_ptr<ILayerRenderer> tmp(renderer);

    size_t index;
    if (LookupLayer(index, source))
    {
      LOG(INFO) << "Renderer ready for layer " << index;
      UpdateLayer(index, tmp.release(), slice);
    }
  }

  
  void LayerWidget::NotifyLayerError(ILayerSource& source,
                                     const SliceGeometry& viewportSlice)
  {
    size_t i;
    if (LookupLayer(i, source))
      LOG(ERROR) << "Error on layer " << i;
  }    


  void LayerWidget::Start()
  {
    if (started_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);      
    }
    
    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      layers_[i]->Start();
    }
  }
}
