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


#pragma once

#include "WorldSceneWidget.h"

#include "../Layers/ILayerRendererFactory.h"

#include <boost/thread/mutex.hpp>  // TODO remove

namespace OrthancStone
{
  class LayeredSceneWidget : public WorldSceneWidget
  {
  public:
    class ISliceObserver : public boost::noncopyable
    {
    public:
      virtual ~ISliceObserver()
      {
      }

      virtual void NotifySliceChange(const LayeredSceneWidget& source,
                                     const SliceGeometry& slice) = 0;
    };

  private:
    struct SliceChangeFunctor;
    class Renderers;
    class PendingLayers;
    class Layer;

    typedef ObserversRegistry<LayeredSceneWidget, ISliceObserver>  Observers;

    std::vector<Layer*>           layers_;
    std::auto_ptr<Renderers>      renderers_;
    std::auto_ptr<PendingLayers>  pendingLayers_;
    std::auto_ptr<Renderers>      pendingRenderers_;
    boost::mutex                  sliceMutex_;
    SliceGeometry                 slice_;
    Observers                     observers_;

  protected:
    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view);

  public:
    LayeredSceneWidget();

    virtual ~LayeredSceneWidget();

    virtual SliceGeometry GetSlice();

    virtual void GetSceneExtent(double& x1,
                                double& y1,
                                double& x2,
                                double& y2);

    ILayerRendererFactory& AddLayer(size_t& layerIndex,
                                    ILayerRendererFactory* factory);   // Takes ownership

    // Simpler version for basic use cases
    void AddLayer(ILayerRendererFactory* factory);   // Takes ownership

    size_t GetLayerCount() const
    {
      return layers_.size();
    }

    RenderStyle GetLayerStyle(size_t layer);
    
    void SetLayerStyle(size_t layer,
                       const RenderStyle& style);

    void SetSlice(const SliceGeometry& slice);

    void InvalidateLayer(unsigned int layer);

    void InvalidateAllLayers();

    virtual void Start();

    virtual void Stop();

    using WorldSceneWidget::Register;
    using WorldSceneWidget::Unregister;

    void Register(ISliceObserver& observer)
    {
      observers_.Register(observer);
    }

    void Unregister(ISliceObserver& observer)
    {
      observers_.Unregister(observer);
    }

    virtual bool HasUpdateContent() const
    {
      return true;
    }

    virtual void UpdateContent();
  };
}
