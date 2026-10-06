/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#include "GrayscaleWindowingSceneTracker.h"

#include "../Scene2DViewport/ViewportController.h"
#include "FloatTextureSceneLayer.h"

#include <OrthancException.h>

namespace OrthancStone
{
  namespace
  {
    class GrayscaleLayerAccessor : public boost::noncopyable
    {
    private:
      FloatTextureSceneLayer*  layer_;

    public:
      GrayscaleLayerAccessor(Scene2D& scene,
                             int layerIndex) :
        layer_(NULL)
      {
        if (scene.HasLayer(layerIndex))
        {
          ISceneLayer& layer = scene.GetLayer(layerIndex);
          if (layer.GetType() == ISceneLayer::Type_FloatTexture)
          {
            layer_ = &dynamic_cast<FloatTextureSceneLayer&>(layer);
          }
        }
      }

      bool IsValid() const
      {
        return layer_ != NULL;
      }

      FloatTextureSceneLayer& GetLayer() const
      {
        if (layer_ == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }
        else
        {
          return *layer_;
        }
      }
    };
  }
  
  bool GrayscaleWindowingSceneTracker::SetWindowing(Scene2D& scene,
                                                    float center,
                                                    float width)
  {
    if (active_)
    {
      GrayscaleLayerAccessor accessor(scene, layerIndex_);
      
      if (accessor.IsValid())
      {
        accessor.GetLayer().SetCustomWindowing(center, width);

        if (HasObserver())
        {
          GetObserver().HandleGrayscaleWindowingChange(Windowing(center, width));
        }

        return true;
      }
    }

    return false;
  }
    

  GrayscaleWindowingSceneTracker::GrayscaleWindowingSceneTracker(Scene2D& scene,
                                                                 int layerIndex,
                                                                 const PointerEvent& event,
                                                                 unsigned int canvasWidth,
                                                                 unsigned int canvasHeight) :
    layerIndex_(layerIndex),
    clickX_(event.GetMainPosition().GetX()),
    clickY_(event.GetMainPosition().GetY())
  {
    active_ = false;

    if (canvasWidth > 3 &&
        canvasHeight > 3)
    {
      GrayscaleLayerAccessor accessor(scene, layerIndex_);

      if (accessor.IsValid())
      {
        accessor.GetLayer().GetWindowing(originalCenter_, originalWidth_);
        
        float minValue, maxValue;
        accessor.GetLayer().GetRange(minValue, maxValue);
        
        normalization_ = (maxValue - minValue) / static_cast<double>(std::min(canvasWidth, canvasHeight) - 1);
        active_ = true;
      }
      else
      {
        LOG(INFO) << "Cannot create GrayscaleWindowingSceneTracker on a non-float texture";
      }
    }
  }

  bool GrayscaleWindowingSceneTracker::PointerMove(const PointerEvent& event,
                                                   Scene2D& scene)
  {
    if (active_)
    {
      const double x = event.GetMainPosition().GetX();
      const double y = event.GetMainPosition().GetY();

      float center = originalCenter_ + static_cast<float>((x - clickX_) * normalization_);
      float width = originalWidth_ + static_cast<float>((y - clickY_) * normalization_);

      if (width <= 1)
      {
        width = 1;
      }
      
      return SetWindowing(scene, center, width);
    }
    else
    {
      return false;
    }
  }

  void GrayscaleWindowingSceneTracker::Cancel(Scene2D& scene)
  {
    SetWindowing(scene, originalCenter_, originalWidth_);
  }
}
