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
      std::unique_ptr<IViewport::ILock>   lock_;
      FloatTextureSceneLayer*             layer_;

    public:
      GrayscaleLayerAccessor(boost::shared_ptr<IViewport> viewport,
                             int layerIndex) :
        layer_(NULL)
      {
        if (viewport != NULL)
        {
          lock_.reset(viewport->Lock());

          if (lock_->GetController().GetScene().HasLayer(layerIndex))
          {
            ISceneLayer& layer = lock_->GetController().GetScene().GetLayer(layerIndex);
            if (layer.GetType() == ISceneLayer::Type_FloatTexture)
            {
              layer_ = &dynamic_cast<FloatTextureSceneLayer&>(layer);
            }
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

      void Invalidate()
      {
        if (lock_.get() != NULL)
        {
          lock_->Invalidate();
        }
      }
    };
  }
  
  void GrayscaleWindowingSceneTracker::SetWindowing(float center,
                                                    float width)
  {
    if (active_)
    {
      GrayscaleLayerAccessor accessor(viewport_, layerIndex_);
      
      if (accessor.IsValid())
      {
        accessor.GetLayer().SetCustomWindowing(center, width);
        accessor.Invalidate();
      }
    }      
  }
    

  GrayscaleWindowingSceneTracker::GrayscaleWindowingSceneTracker(boost::shared_ptr<IViewport> viewport,
                                                                 int layerIndex,
                                                                 const PointerEvent& event,
                                                                 unsigned int canvasWidth,
                                                                 unsigned int canvasHeight) :
    OneGesturePointerTracker(viewport),
    layerIndex_(layerIndex),
    clickX_(event.GetMainPosition().GetX()),
    clickY_(event.GetMainPosition().GetY())
  {
    active_ = false;

    if (canvasWidth > 3 &&
        canvasHeight > 3)
    {
      GrayscaleLayerAccessor accessor(viewport_, layerIndex_);
      
      if (accessor.IsValid())
      {
        FloatTextureSceneLayer& layer = accessor.GetLayer();
        
        layer.GetWindowing(originalCenter_, originalWidth_);
        
        float minValue, maxValue;
        layer.GetRange(minValue, maxValue);
        
        normalization_ = (maxValue - minValue) / static_cast<double>(std::min(canvasWidth, canvasHeight) - 1);
        active_ = true;
      }
      else
      {
        LOG(INFO) << "Cannot create GrayscaleWindowingSceneTracker on a non-float texture";
      }
    }
  }
  
  void GrayscaleWindowingSceneTracker::PointerMove(const PointerEvent& event)
  {
    if (active_)
    {
      const double x = event.GetMainPosition().GetX();
      const double y = event.GetMainPosition().GetY();

      float center = originalCenter_ + (x - clickX_) * normalization_;
      float width = originalWidth_ + (y - clickY_) * normalization_;

      if (width <= 1)
      {
        width = 1;
      }
      
      SetWindowing(center, width);
    }
  }

  void GrayscaleWindowingSceneTracker::Cancel()
  {
    SetWindowing(originalCenter_, originalWidth_);
  }
}