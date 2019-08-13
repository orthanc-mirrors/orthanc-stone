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

#include "LayerHolder.h"
#include "../Scene2D/TextSceneLayer.h"
#include "../Scene2D/PolylineSceneLayer.h"
#include "../Scene2D/Scene2D.h"
#include "../Scene2DViewport/ViewportController.h"
#include "../StoneException.h"

namespace OrthancStone
{
  LayerHolder::LayerHolder(
    boost::weak_ptr<ViewportController> controllerW,
    int                    polylineLayerCount,
    int                    textLayerCount,
    int                    infoTextCount)
    : textLayerCount_(textLayerCount)
    , polylineLayerCount_(polylineLayerCount)
    , infoTextCount_(infoTextCount)
    , controllerW_(controllerW)
    , baseLayerIndex_(-1)
  {

  }

  void LayerHolder::CreateLayers()
  {
    assert(baseLayerIndex_ == -1);

    baseLayerIndex_ = GetScene().GetMaxDepth() + 100;

    for (int i = 0; i < polylineLayerCount_; ++i)
    {
      std::auto_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer());
      GetScene().SetLayer(baseLayerIndex_ + i, layer.release());
    }

    for (int i = 0; i < textLayerCount_; ++i)
    {
      std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer());
      GetScene().SetLayer(
        baseLayerIndex_ + polylineLayerCount_ + i,
        layer.release());
    }

  }

  void LayerHolder::CreateLayersIfNeeded()
  {
    if (baseLayerIndex_ == -1)
      CreateLayers();
  }

  bool LayerHolder::AreLayersCreated() const
  {
    return (baseLayerIndex_ != -1);
  }

  Scene2D& LayerHolder::GetScene()
  {
    boost::shared_ptr<ViewportController> controller = controllerW_.lock();
    ORTHANC_ASSERT(controller.get() != 0, "Zombie attack!");
    return controller->GetScene();
  }

  void LayerHolder::DeleteLayersIfNeeded()
  {
    if (baseLayerIndex_ != -1)
      DeleteLayers();
  }
  
  void LayerHolder::DeleteLayers()
  {
    for (int i = 0; i < textLayerCount_ + polylineLayerCount_; ++i)
    {
      ORTHANC_ASSERT(GetScene().HasLayer(baseLayerIndex_ + i), "No layer");
      GetScene().DeleteLayer(baseLayerIndex_ + i);
    }
    baseLayerIndex_ = -1;
  }

  PolylineSceneLayer* LayerHolder::GetPolylineLayer(int index /*= 0*/)
  {
    using namespace Orthanc;
    ORTHANC_ASSERT(baseLayerIndex_ != -1);
    ORTHANC_ASSERT(GetScene().HasLayer(GetPolylineLayerIndex(index)));
    ISceneLayer* layer =
      &(GetScene().GetLayer(GetPolylineLayerIndex(index)));

    PolylineSceneLayer* concreteLayer =
      dynamic_cast<PolylineSceneLayer*>(layer);

    ORTHANC_ASSERT(concreteLayer != NULL);
    return concreteLayer;
  }

  TextSceneLayer* LayerHolder::GetTextLayer(int index /*= 0*/)
  {
    using namespace Orthanc;
    ORTHANC_ASSERT(baseLayerIndex_ != -1);
    ORTHANC_ASSERT(GetScene().HasLayer(GetTextLayerIndex(index)));
    ISceneLayer* layer =
      &(GetScene().GetLayer(GetTextLayerIndex(index)));

    TextSceneLayer* concreteLayer =
      dynamic_cast<TextSceneLayer*>(layer);

    ORTHANC_ASSERT(concreteLayer != NULL);
    return concreteLayer;
  }

  int LayerHolder::GetPolylineLayerIndex(int index /*= 0*/)
  {
    using namespace Orthanc;
    ORTHANC_ASSERT(index < polylineLayerCount_);
    return baseLayerIndex_ + index;
  }


  int LayerHolder::GetTextLayerIndex(int index /*= 0*/)
  {
    using namespace Orthanc;
    ORTHANC_ASSERT(index < textLayerCount_);

    // the text layers are placed right after the polyline layers
    // this means they are drawn ON TOP
    return baseLayerIndex_ + polylineLayerCount_ + index;
  }
}
