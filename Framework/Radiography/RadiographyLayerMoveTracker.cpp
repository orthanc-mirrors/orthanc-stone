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


#include "RadiographyLayerMoveTracker.h"

#include "RadiographySceneCommand.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  class RadiographyLayerMoveTracker::UndoRedoCommand : public RadiographySceneCommand
  {
  private:
    double  sourceX_;
    double  sourceY_;
    double  targetX_;
    double  targetY_;

  protected:
    virtual void UndoInternal(RadiographyLayer& layer) const
    {
      layer.SetPan(sourceX_, sourceY_);
    }

    virtual void RedoInternal(RadiographyLayer& layer) const
    {
      layer.SetPan(targetX_, targetY_);
    }

  public:
    UndoRedoCommand(const RadiographyLayerMoveTracker& tracker) :
      RadiographySceneCommand(tracker.accessor_),
      sourceX_(tracker.panX_),
      sourceY_(tracker.panY_),
      targetX_(tracker.accessor_.GetLayer().GetPanX()),
      targetY_(tracker.accessor_.GetLayer().GetPanY())
    {
    }
  };


  RadiographyLayerMoveTracker::RadiographyLayerMoveTracker(UndoRedoStack& undoRedoStack,
                                                           RadiographyScene& scene,
                                                           size_t layer,
                                                           double x,
                                                           double y,
                                                           bool oneAxis) :
    undoRedoStack_(undoRedoStack),
    accessor_(scene, layer),
    clickX_(x),
    clickY_(y),
    oneAxis_(oneAxis)
  {
    if (accessor_.IsValid())
    {
      panX_ = accessor_.GetLayer().GetPanX();
      panY_ = accessor_.GetLayer().GetPanY();
    }
  }


  void RadiographyLayerMoveTracker::Render(CairoContext& context,
                                           double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }

  
  void RadiographyLayerMoveTracker::MouseUp()
  {
    if (accessor_.IsValid())
    {
      undoRedoStack_.Add(new UndoRedoCommand(*this));
    }
  }

  
  void RadiographyLayerMoveTracker::MouseMove(int displayX,
                                              int displayY,
                                              double sceneX,
                                              double sceneY)
  {
    if (accessor_.IsValid())
    {
      double dx = sceneX - clickX_;
      double dy = sceneY - clickY_;

      if (oneAxis_)
      {
        if (fabs(dx) > fabs(dy))
        {
          accessor_.GetLayer().SetPan(dx + panX_, panY_);
        }
        else
        {
          accessor_.GetLayer().SetPan(panX_, dy + panY_);
        }
      }
      else
      {
        accessor_.GetLayer().SetPan(dx + panX_, dy + panY_);
      }
    }
  }
}
