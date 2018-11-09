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

#include "WorldSceneWidget.h"
#include "../Layers/ILayerSource.h"
#include "../Toolbox/Extent2D.h"
#include "../../Framework/Messages/IObserver.h"

#include <map>

namespace OrthancStone
{
  class LayerWidget :
      public WorldSceneWidget,
      public IObserver,
      public IObservable
  {
  public:
    typedef OriginMessage<MessageType_Widget_GeometryChanged, LayerWidget> GeometryChangedMessage;
    typedef OriginMessage<MessageType_Widget_ContentChanged, LayerWidget> ContentChangedMessage;

  private:
    class Scene;
    
    typedef std::map<const ILayerSource*, size_t>  LayersIndex;

    bool                        started_;
    LayersIndex                 layersIndex_;
    std::vector<ILayerSource*>  layers_;
    std::vector<RenderStyle>    styles_;
    CoordinateSystem3D          slice_;
    std::auto_ptr<Scene>        currentScene_;
    std::auto_ptr<Scene>        pendingScene_;
    std::vector<bool>           changedLayers_;

    bool LookupLayer(size_t& index /* out */,
                     const ILayerSource& layer) const;

    void GetLayerExtent(Extent2D& extent,
                        ILayerSource& source) const;

    void OnGeometryReady(const ILayerSource::GeometryReadyMessage& message);

    virtual void OnContentChanged(const ILayerSource::ContentChangedMessage& message);

    virtual void OnSliceChanged(const ILayerSource::SliceChangedMessage& message);

    virtual void OnLayerReady(const ILayerSource::LayerReadyMessage& message);

    virtual void OnLayerError(const ILayerSource::LayerErrorMessage& message);

    void ObserveLayer(ILayerSource& source);

    void ResetChangedLayers();

  public:
    LayerWidget(MessageBroker& broker, const std::string& name);

    virtual Extent2D GetSceneExtent();

  protected:
    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view);

    void ResetPendingScene();

    void UpdateLayer(size_t index,
                     ILayerRenderer* renderer,
                     const CoordinateSystem3D& slice);

    void InvalidateAllLayers();

    void InvalidateLayer(size_t layer);
    
  public:
    virtual ~LayerWidget();

    size_t AddLayer(ILayerSource* layer);  // Takes ownership

    void ReplaceLayer(size_t layerIndex, ILayerSource* layer); // Takes ownership

    void RemoveLayer(size_t layerIndex);

    size_t GetLayerCount() const
    {
      return layers_.size();
    }

    const RenderStyle& GetLayerStyle(size_t layer) const;

    void SetLayerStyle(size_t layer,
                       const RenderStyle& style);

    void SetSlice(const CoordinateSystem3D& slice);

    const CoordinateSystem3D& GetSlice() const
    {
      return slice_;
    }

    virtual bool HasAnimation() const
    {
      return true;
    }

    virtual void DoAnimation();
  };
}
