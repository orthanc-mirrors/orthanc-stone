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

#include "../Widgets/WorldSceneWidget.h"
#include "RadiographyScene.h"


namespace OrthancStone
{
  class RadiographyWidget :
    public WorldSceneWidget,
    public IObserver
  {
  private:
    RadiographyScene&                      scene_;
    std::auto_ptr<Orthanc::ImageAccessor>  floatBuffer_;
    std::auto_ptr<CairoSurface>            cairoBuffer_;
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
      return scene_.GetSceneExtent();
    }

    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view);

  public:
    RadiographyWidget(MessageBroker& broker,
                      RadiographyScene& scene,
                      const std::string& name);

    RadiographyScene& GetScene() const
    {
      return scene_;
    }

    void Unselect()
    {
      hasSelection_ = false;
    }

    void Select(size_t layer);

    bool LookupSelectedLayer(size_t& layer);

    void OnGeometryChanged(const RadiographyScene::GeometryChangedMessage& message);

    void OnContentChanged(const RadiographyScene::ContentChangedMessage& message);

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
}
