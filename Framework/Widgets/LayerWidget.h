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
#include "../Layers/ILayerSource.h"

#include <map>

namespace OrthancStone
{
  class LayerWidget :
    public WorldSceneWidget,
    public ILayerSource::IObserver   // TODO move this as PImpl
  {
  private:
    class Scene;
    
    typedef std::map<ILayerSource*, size_t>  LayersIndex;

    bool                        started_;
    LayersIndex                 layersIndex_;
    std::vector<ILayerSource*>  layers_;
    std::vector<RenderStyle>    styles_;
    SliceGeometry               slice_;
    double                      sliceThickness_;
    std::auto_ptr<Scene>        currentScene_;
    std::auto_ptr<Scene>        pendingScene_;


    bool LookupLayer(size_t& index /* out */,
                     ILayerSource& layer) const;
    
        
  protected:
    virtual void GetSceneExtent(double& x1,
                                double& y1,
                                double& x2,
                                double& y2);
 
    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view);

    void ResetPendingScene();

    void UpdateLayer(size_t index,
                     ILayerRenderer* renderer,
                     const SliceGeometry& slice);
    
  public:
    LayerWidget();

    virtual ~LayerWidget();

    size_t AddLayer(ILayerSource* layer);  // Takes ownership

    void SetLayerStyle(size_t layer,
                       const RenderStyle& style);

    void SetSlice(const SliceGeometry& slice,
                  double sliceThickness);

    virtual void NotifyGeometryReady(ILayerSource& source);

    virtual void NotifySourceChange(ILayerSource& source);

    virtual void NotifySliceChange(ILayerSource& source,
                                   const SliceGeometry& slice);

    virtual void NotifyLayerReady(ILayerRenderer* renderer,
                                  ILayerSource& source,
                                  const SliceGeometry& viewportSlice);

    virtual void NotifyLayerError(ILayerSource& source,
                                  const SliceGeometry& viewportSlice);

    virtual void Start();
  };
}
