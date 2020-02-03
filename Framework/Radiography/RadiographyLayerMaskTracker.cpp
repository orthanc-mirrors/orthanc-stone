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


#include "RadiographyLayerMaskTracker.h"
#include "RadiographyMaskLayer.h"

#include "RadiographySceneCommand.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  class RadiographyLayerMaskTracker::UndoRedoCommand : public RadiographySceneCommand
  {
  private:
    ControlPoint sourceSceneCp_;
    ControlPoint targetSceneCp_;

  protected:
    virtual void UndoInternal(RadiographyLayer& layer) const
    {
      RadiographyMaskLayer* maskLayer = dynamic_cast<RadiographyMaskLayer*>(&layer);
      if (maskLayer == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      unsigned int ix, iy; // image coordinates
      if (maskLayer->GetPixel(ix, iy, sourceSceneCp_.x, sourceSceneCp_.y))
      {
        maskLayer->SetCorner(Orthanc::ImageProcessing::ImagePoint((int32_t)ix, (int32_t)iy), sourceSceneCp_.index);
      }
    }

    virtual void RedoInternal(RadiographyLayer& layer) const
    {
      RadiographyMaskLayer* maskLayer = dynamic_cast<RadiographyMaskLayer*>(&layer);
      if (maskLayer == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      unsigned int ix, iy; // image coordinates
      if (maskLayer->GetPixel(ix, iy, targetSceneCp_.x, targetSceneCp_.y))
      {
        maskLayer->SetCorner(Orthanc::ImageProcessing::ImagePoint((int32_t)ix, (int32_t)iy), targetSceneCp_.index);
      }
    }

  public:
    UndoRedoCommand(const RadiographyLayerMaskTracker& tracker) :
      RadiographySceneCommand(tracker.accessor_),
      sourceSceneCp_(tracker.startSceneCp_),
      targetSceneCp_(tracker.endSceneCp_)
    {
      RadiographyMaskLayer* maskLayer = dynamic_cast<RadiographyMaskLayer*>(&(tracker.accessor_.GetLayer()));
      if (maskLayer == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      unsigned int ix, iy; // image coordinates
      if (maskLayer->GetPixel(ix, iy, targetSceneCp_.x, targetSceneCp_.y))
      {
        maskLayer->SetCorner(Orthanc::ImageProcessing::ImagePoint((int32_t)ix, (int32_t)iy), targetSceneCp_.index);
      }
    }
  };


  RadiographyLayerMaskTracker::RadiographyLayerMaskTracker(UndoRedoStack& undoRedoStack,
                                                           RadiographyScene& scene,
                                                           const Deprecated::ViewportGeometry& view,
                                                           size_t layer,
                                                           const ControlPoint& startSceneControlPoint) :
    undoRedoStack_(undoRedoStack),
    accessor_(scene, layer),
    startSceneCp_(startSceneControlPoint),
    endSceneCp_(startSceneControlPoint)
  {
  }


  void RadiographyLayerMaskTracker::Render(CairoContext& context,
                                           double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void RadiographyLayerMaskTracker::MouseUp()
  {
    if (accessor_.IsValid() && startSceneCp_.x != endSceneCp_.x && startSceneCp_.y != endSceneCp_.y)
    {
      undoRedoStack_.Add(new UndoRedoCommand(*this));
    }
  }

  
  void RadiographyLayerMaskTracker::MouseMove(int displayX,
                                              int displayY,
                                              double sceneX,
                                              double sceneY,
                                              const std::vector<Deprecated::Touch>& displayTouches,
                                              const std::vector<Deprecated::Touch>& sceneTouches)
  {
    if (accessor_.IsValid())
    {
      unsigned int ix, iy; // image coordinates

      RadiographyLayer& layer = accessor_.GetLayer();
      if (layer.GetPixel(ix, iy, sceneX, sceneY))
      {
        endSceneCp_ = ControlPoint(sceneX, sceneY, startSceneCp_.index);

        RadiographyMaskLayer* maskLayer = dynamic_cast<RadiographyMaskLayer*>(&layer);
        if (maskLayer == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
        }
        maskLayer->SetCorner(Orthanc::ImageProcessing::ImagePoint((int32_t)ix, (int32_t)iy), startSceneCp_.index);
      }
    }
  }
}
