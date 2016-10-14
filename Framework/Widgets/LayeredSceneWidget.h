/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include "WorldSceneWidget.h"

#include "../Layers/ILayerRendererFactory.h"


namespace OrthancStone
{
  class LayeredSceneWidget : public WorldSceneWidget
  {
  public:
    // Must be thread-safe
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
    virtual bool HasUpdateThread() const
    {
      return true;
    }

    virtual void UpdateStep();

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
  };
}
