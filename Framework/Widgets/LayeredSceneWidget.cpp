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


#define _USE_MATH_DEFINES  // To access M_PI in Visual Studio
#include <cmath>

#include "LayeredSceneWidget.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"

#include <boost/thread/condition_variable.hpp>   // TODO Remove

namespace OrthancStone
{
  class LayeredSceneWidget::Renderers : public boost::noncopyable
  {
  private:
    boost::mutex                  mutex_;
    std::vector<ILayerRenderer*>  renderers_;
    std::vector<bool>             assigned_;
      
    void Assign(size_t index,
                ILayerRenderer* renderer)
    {
      if (index >= renderers_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
        
      if (renderers_[index] != NULL)
      {
        delete renderers_[index];
      }

      renderers_[index] = renderer;
      assigned_[index] = true;
    }

  public:
    Renderers(size_t size)
    {
      renderers_.resize(size);
      assigned_.resize(size, false);
    }

    ~Renderers()
    {
      for (size_t i = 0; i < renderers_.size(); i++)
      {
        Assign(i, NULL);
      }
    }

    static void Merge(Renderers& target,
                      Renderers& source)
    {
      boost::mutex::scoped_lock lockSource(source.mutex_);
      boost::mutex::scoped_lock lockTarget(target.mutex_);

      size_t count = target.renderers_.size();
      if (count != source.renderers_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      for (size_t i = 0; i < count; i++)
      {
        if (source.assigned_[i])
        {
          target.Assign(i, source.renderers_[i]);  // Transfers ownership
          source.renderers_[i] = NULL;
          source.assigned_[i] = false;
        }
      }
    }

    void SetRenderer(size_t index,
                     ILayerRenderer* renderer)  // Takes ownership
    {
      boost::mutex::scoped_lock lock(mutex_);
      Assign(index, renderer);
    }

    bool RenderScene(CairoContext& context,
                     const ViewportGeometry& view)
    {
      boost::mutex::scoped_lock lock(mutex_);

      bool fullQuality = true;

      for (size_t i = 0; i < renderers_.size(); i++)
      {
        if (renderers_[i] != NULL &&
            !renderers_[i]->RenderLayer(context, view))
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
      boost::mutex::scoped_lock lock(mutex_);
        
      if (renderers_[index] != NULL)
      {
        renderers_[index]->SetLayerStyle(style);
      }
    }
  };



  class LayeredSceneWidget::PendingLayers : public boost::noncopyable
  {
  private:
    boost::mutex               mutex_;
    boost::condition_variable  elementAvailable_;
    size_t                     layerCount_;
    std::list<size_t>          queue_;
    std::vector<bool>          layersToUpdate_;
    bool                       continue_;

    void TagAllLayers()
    {
      queue_.clear();

      for (unsigned int i = 0; i < layerCount_; i++)
      {
        queue_.push_back(i);
        layersToUpdate_[i] = true;
      }

      if (layerCount_ != 0)
      {
        elementAvailable_.notify_one();
      }
    }
      
  public:
    PendingLayers() : 
      layerCount_(0), 
      continue_(true)
    {
    }

    void Stop()
    {
      continue_ = false;
      elementAvailable_.notify_one();
    }

    void SetLayerCount(size_t count)
    {
      boost::mutex::scoped_lock lock(mutex_);

      layerCount_ = count;
      layersToUpdate_.resize(count);

      TagAllLayers();
    }

    void InvalidateAllLayers()
    {
      boost::mutex::scoped_lock lock(mutex_);
      TagAllLayers();
    }

    void InvalidateLayer(size_t layer)
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (layer < layerCount_)
      {
        if (layersToUpdate_[layer])
        {
          // The layer is already scheduled for update, ignore this
          // invalidation
        }
        else
        {
          queue_.push_back(layer);
          layersToUpdate_[layer] = true;
          elementAvailable_.notify_one();
        }
      }
    }

    bool Dequeue(size_t& layer,
                 bool& isLast)
    {
      boost::mutex::scoped_lock lock(mutex_);

      // WARNING: Do NOT use "timed_wait" on condition variables, as
      // sleeping is not properly supported by Boost for Google NaCl
      while (queue_.empty() && 
             continue_)
      {
        elementAvailable_.wait(lock);
      }

      if (!continue_)
      {
        return false;
      }

      layer = queue_.front();
      layersToUpdate_[layer] = false;
      queue_.pop_front();

      isLast = queue_.empty();

      return true;
    }
  };


  class LayeredSceneWidget::Layer : public ISliceableVolume::IChangeObserver
  {
  private:
    boost::mutex                          mutex_;
    std::auto_ptr<ILayerRendererFactory>  factory_;
    PendingLayers&                        layers_;
    size_t                                index_;
    std::auto_ptr<RenderStyle>            style_;

  public:
    Layer(ILayerRendererFactory*  factory,
          PendingLayers& layers,
          size_t index) :
      factory_(factory),
      layers_(layers),
      index_(index)
    {
      if (factory == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    virtual void NotifyChange(const OrthancStone::ISliceableVolume&)
    {
      layers_.InvalidateLayer(index_);
    }

    void Start()
    {
      if (factory_->HasSourceVolume())
      {
        factory_->GetSourceVolume().Register(*this);
      }
    }

    void Stop()
    {
      if (factory_->HasSourceVolume())
      {
        factory_->GetSourceVolume().Unregister(*this);
      }
    }

    bool GetExtent(double& x1,
                   double& y1,
                   double& x2,
                   double& y2,
                   const SliceGeometry& displaySlice) 
    {
      boost::mutex::scoped_lock lock(mutex_);
      assert(factory_.get() != NULL);
      return factory_->GetExtent(x1, y1, x2, y2, displaySlice);
    }

    RenderStyle GetStyle() 
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (style_.get() == NULL)
      {
        return RenderStyle();
      }
      else
      {
        return *style_;
      }
    }

    void SetStyle(const RenderStyle& style)
    {
      boost::mutex::scoped_lock lock(mutex_);
      style_.reset(new RenderStyle(style));
    }


    ILayerRenderer* CreateRenderer(const SliceGeometry& displaySlice)
    {
      boost::mutex::scoped_lock lock(mutex_);
      assert(factory_.get() != NULL);

      std::auto_ptr<ILayerRenderer> renderer(factory_->CreateLayerRenderer(displaySlice));
        
      if (renderer.get() != NULL &&
          style_.get() != NULL)
      {
        renderer->SetLayerStyle(*style_);
      }

      return renderer.release();
    }
  };



  SliceGeometry LayeredSceneWidget::GetSlice()
  {
    boost::mutex::scoped_lock lock(sliceMutex_);
    return slice_;
  }


  void LayeredSceneWidget::UpdateContent()
  {
    size_t layer = 0;
    bool isLast = true;
    if (!pendingLayers_->Dequeue(layer, isLast))
    {
      return;
    }

    SliceGeometry slice = GetSlice();

    std::auto_ptr<ILayerRenderer> renderer;
    renderer.reset(layers_[layer]->CreateRenderer(slice));

    if (renderer.get() != NULL)
    {
      pendingRenderers_->SetRenderer(layer, renderer.release());
    }
    else
    {
      pendingRenderers_->SetRenderer(layer, NULL);
    }

    if (isLast)
    {
      Renderers::Merge(*renderers_, *pendingRenderers_);
      NotifyChange();
    }

    // TODO Add sleep at this point
  }
    

  bool LayeredSceneWidget::RenderScene(CairoContext& context,
                                       const ViewportGeometry& view) 
  {
    return renderers_->RenderScene(context, view);
  }


  LayeredSceneWidget::LayeredSceneWidget() 
  {
    pendingLayers_.reset(new PendingLayers);
    SetBackgroundCleared(true);
  }


  LayeredSceneWidget::~LayeredSceneWidget()
  {
    for (size_t i = 0; i < layers_.size(); i++)
    {
      assert(layers_[i] != NULL);
      delete layers_[i];
    }
  }


  void LayeredSceneWidget::GetSceneExtent(double& x1,
                                          double& y1,
                                          double& x2,
                                          double& y2)
  {
    boost::mutex::scoped_lock lock(sliceMutex_);

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



  ILayerRendererFactory& LayeredSceneWidget::AddLayer(size_t& layerIndex,
                                                      ILayerRendererFactory* factory)
  {
    layerIndex = layers_.size();
    layers_.push_back(new Layer(factory, *pendingLayers_, layers_.size()));

    return *factory;
  }


  void LayeredSceneWidget::AddLayer(ILayerRendererFactory* factory)
  {
    size_t layerIndex;  // Ignored
    AddLayer(layerIndex, factory);
  }


  RenderStyle LayeredSceneWidget::GetLayerStyle(size_t layer)
  {
    if (layer >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    return layers_[layer]->GetStyle();
  }


  void LayeredSceneWidget::SetLayerStyle(size_t layer,
                                         const RenderStyle& style)
  {
    if (layer >= layers_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    layers_[layer]->SetStyle(style);

    if (renderers_.get() != NULL)
    {
      renderers_->SetLayerStyle(layer, style);
    }

    InvalidateLayer(layer);
  }



  struct LayeredSceneWidget::SliceChangeFunctor
  {
    const SliceGeometry& slice_;

    SliceChangeFunctor(const SliceGeometry& slice) :
      slice_(slice)
    {
    }

    void operator() (ISliceObserver& observer,
                     const LayeredSceneWidget& source)
    {
      observer.NotifySliceChange(source, slice_);
    }
  };


  void LayeredSceneWidget::SetSlice(const SliceGeometry& slice)
  {
    { 
      boost::mutex::scoped_lock lock(sliceMutex_);
      slice_ = slice;
    }

    InvalidateAllLayers();

    SliceChangeFunctor functor(slice);
    observers_.Notify(this, functor);
  }


  void LayeredSceneWidget::InvalidateLayer(unsigned int layer)
  {
    pendingLayers_->InvalidateLayer(layer);
    //NotifyChange();  // TODO Understand why this makes the SDL engine not update the display subsequently
  }


  void LayeredSceneWidget::InvalidateAllLayers()
  {
    pendingLayers_->InvalidateAllLayers();
    //NotifyChange();  // TODO Understand why this makes the SDL engine not update the display subsequently
  }


#if 0
  void LayeredSceneWidget::Start()
  {
    for (size_t i = 0; i < layers_.size(); i++)
    {
      layers_[i]->Start();
    }

    renderers_.reset(new Renderers(layers_.size()));
    pendingRenderers_.reset(new Renderers(layers_.size()));

    pendingLayers_->SetLayerCount(layers_.size());

    WorldSceneWidget::Start();
  }


  void LayeredSceneWidget::Stop()
  {
    pendingLayers_->Stop();

    renderers_.reset(NULL);
    pendingRenderers_.reset(NULL);

    for (size_t i = 0; i < layers_.size(); i++)
    {
      layers_[i]->Stop();
    }
  }
#endif
}
