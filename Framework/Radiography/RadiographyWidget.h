/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include "../Deprecated/Widgets/WorldSceneWidget.h"
#include "RadiographyScene.h"


namespace OrthancStone
{
  class RadiographyMaskLayer;

  class RadiographyWidget :
    public Deprecated::WorldSceneWidget,
    public IObserver,
    public IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, SelectionChangedMessage, RadiographyWidget);

  private:
    boost::shared_ptr<RadiographyScene>    scene_;
    std::unique_ptr<Orthanc::ImageAccessor>  floatBuffer_;
    std::unique_ptr<CairoSurface>            cairoBuffer_;
    bool                                   invert_;
    ImageInterpolation                     interpolation_;
    bool                                   hasSelection_;
    size_t                                 selectedLayer_;

    bool RenderInternal(unsigned int width,
                        unsigned int height,
                        ImageInterpolation interpolation);

  protected:
    virtual Extent2D GetSceneExtent()
    {
      return scene_->GetSceneExtent(false);
    }

    virtual bool RenderScene(CairoContext& context,
                             const Deprecated::ViewportGeometry& view);

    virtual void RenderBackground(Orthanc::ImageAccessor& image, float minValue, float maxValue);

    bool IsInvertedInternal() const;

  public:
    RadiographyWidget(MessageBroker& broker,
                      boost::shared_ptr<RadiographyScene> scene,  // TODO: check how we can avoid boost::shared_ptr here since we don't want them in the public API (app is keeping a boost::shared_ptr to this right now)
                      const std::string& name);

    RadiographyScene& GetScene() const
    {
      return *scene_;
    }

    void SetScene(boost::shared_ptr<RadiographyScene> scene);

    void Select(size_t layer);

    void Unselect();

    template<typename LayerType> bool SelectLayerByType(size_t index = 0);

    bool LookupSelectedLayer(size_t& layer) const;

    void OnGeometryChanged(const RadiographyScene::GeometryChangedMessage& message);

    void OnContentChanged(const RadiographyScene::ContentChangedMessage& message);

    void OnLayerRemoved(const RadiographyScene::LayerRemovedMessage& message);

    void SetInvert(bool invert);

    void SwitchInvert();

    bool IsInverted() const
    {
      return invert_;
    }

    void SetInterpolation(ImageInterpolation interpolation);

    ImageInterpolation GetInterpolation() const
    {
      return interpolation_;
    }
  };

  template<typename LayerType> bool RadiographyWidget::SelectLayerByType(size_t index)
  {
    std::vector<size_t> layerIndexes;
    size_t count = 0;
    scene_->GetLayersIndexes(layerIndexes);

    for (size_t i = 0; i < layerIndexes.size(); ++i)
    {
      const LayerType* typedLayer = dynamic_cast<const LayerType*>(&(scene_->GetLayer(layerIndexes[i])));
      if (typedLayer != NULL)
      {
        if (count == index)
        {
          Select(layerIndexes[i]);
          return true;
        }
        count++;
      }
    }

    return false;
  }
}
