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


#include "RadiographyLayerCropTracker.h"

#include "RadiographySceneCommand.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  class RadiographyLayerCropTracker::UndoRedoCommand : public RadiographySceneCommand
  {
  private:
    unsigned int  sourceCropX_;
    unsigned int  sourceCropY_;
    unsigned int  sourceCropWidth_;
    unsigned int  sourceCropHeight_;
    unsigned int  targetCropX_;
    unsigned int  targetCropY_;
    unsigned int  targetCropWidth_;
    unsigned int  targetCropHeight_;

  protected:
    virtual void UndoInternal(RadiographyLayer& layer) const
    {
      layer.SetCrop(sourceCropX_, sourceCropY_, sourceCropWidth_, sourceCropHeight_);
    }

    virtual void RedoInternal(RadiographyLayer& layer) const
    {
      layer.SetCrop(targetCropX_, targetCropY_, targetCropWidth_, targetCropHeight_);
    }

  public:
    UndoRedoCommand(const RadiographyLayerCropTracker& tracker) :
      RadiographySceneCommand(tracker.accessor_),
      sourceCropX_(tracker.cropX_),
      sourceCropY_(tracker.cropY_),
      sourceCropWidth_(tracker.cropWidth_),
      sourceCropHeight_(tracker.cropHeight_)
    {
      tracker.accessor_.GetLayer().GetCrop(targetCropX_, targetCropY_,
                                           targetCropWidth_, targetCropHeight_);
    }
  };


  RadiographyLayerCropTracker::RadiographyLayerCropTracker(UndoRedoStack& undoRedoStack,
                                                           RadiographyScene& scene,
                                                           const ViewportGeometry& view,
                                                           size_t layer,
                                                           double x,
                                                           double y,
                                                           Corner corner) :
    undoRedoStack_(undoRedoStack),
    accessor_(scene, layer),
    corner_(corner)
  {
    if (accessor_.IsValid())
    {
      accessor_.GetLayer().GetCrop(cropX_, cropY_, cropWidth_, cropHeight_);
    }
  }


  void RadiographyLayerCropTracker::Render(CairoContext& context,
                                           double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void RadiographyLayerCropTracker::MouseUp()
  {
    if (accessor_.IsValid())
    {
      undoRedoStack_.Add(new UndoRedoCommand(*this));
    }
  }

  
  void RadiographyLayerCropTracker::MouseMove(int displayX,
                                              int displayY,
                                              double sceneX,
                                              double sceneY,
                                              const std::vector<Touch>& displayTouches,
                                              const std::vector<Touch>& sceneTouches)
  {
    if (accessor_.IsValid())
    {
      unsigned int x, y;
        
      RadiographyLayer& layer = accessor_.GetLayer();
      if (layer.GetPixel(x, y, sceneX, sceneY))
      {
        unsigned int targetX, targetWidth;

        if (corner_ == Corner_TopLeft ||
            corner_ == Corner_BottomLeft)
        {
          targetX = std::min(x, cropX_ + cropWidth_);
          targetWidth = cropX_ + cropWidth_ - targetX;
        }
        else
        {
          targetX = cropX_;
          targetWidth = std::max(x, cropX_) - cropX_;
        }

        unsigned int targetY, targetHeight;

        if (corner_ == Corner_TopLeft ||
            corner_ == Corner_TopRight)
        {
          targetY = std::min(y, cropY_ + cropHeight_);
          targetHeight = cropY_ + cropHeight_ - targetY;
        }
        else
        {
          targetY = cropY_;
          targetHeight = std::max(y, cropY_) - cropY_;
        }

        layer.SetCrop(targetX, targetY, targetWidth, targetHeight);
      }
    }
  }
}
