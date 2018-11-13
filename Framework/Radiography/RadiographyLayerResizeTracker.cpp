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


#include "RadiographyLayerResizeTracker.h"

#include "RadiographySceneCommand.h"

#include <Core/OrthancException.h>

#include <boost/math/special_functions/round.hpp>


namespace OrthancStone
{
  static double ComputeDistance(double x1,
                                double y1,
                                double x2,
                                double y2)
  {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return sqrt(dx * dx + dy * dy);
  }
      

  class RadiographyLayerResizeTracker::UndoRedoCommand : public RadiographySceneCommand
  {
  private:
    double   sourceSpacingX_;
    double   sourceSpacingY_;
    double   sourcePanX_;
    double   sourcePanY_;
    double   targetSpacingX_;
    double   targetSpacingY_;
    double   targetPanX_;
    double   targetPanY_;

  protected:
    virtual void UndoInternal(RadiographyLayer& layer) const
    {
      layer.SetPixelSpacing(sourceSpacingX_, sourceSpacingY_);
      layer.SetPan(sourcePanX_, sourcePanY_);
    }

    virtual void RedoInternal(RadiographyLayer& layer) const
    {
      layer.SetPixelSpacing(targetSpacingX_, targetSpacingY_);
      layer.SetPan(targetPanX_, targetPanY_);
    }

  public:
    UndoRedoCommand(const RadiographyLayerResizeTracker& tracker) :
      RadiographySceneCommand(tracker.accessor_),
      sourceSpacingX_(tracker.originalSpacingX_),
      sourceSpacingY_(tracker.originalSpacingY_),
      sourcePanX_(tracker.originalPanX_),
      sourcePanY_(tracker.originalPanY_),
      targetSpacingX_(tracker.accessor_.GetLayer().GetPixelSpacingX()),
      targetSpacingY_(tracker.accessor_.GetLayer().GetPixelSpacingY()),
      targetPanX_(tracker.accessor_.GetLayer().GetPanX()),
      targetPanY_(tracker.accessor_.GetLayer().GetPanY())
    {
    }
  };


  RadiographyLayerResizeTracker::RadiographyLayerResizeTracker(UndoRedoStack& undoRedoStack,
                                                               RadiographyScene& scene,
                                                               size_t layer,
                                                               double x,
                                                               double y,
                                                               Corner corner,
                                                               bool roundScaling) :
    undoRedoStack_(undoRedoStack),
    accessor_(scene, layer),
    roundScaling_(roundScaling)
  {
    if (accessor_.IsValid() &&
        accessor_.GetLayer().IsResizeable())
    {
      originalSpacingX_ = accessor_.GetLayer().GetPixelSpacingX();
      originalSpacingY_ = accessor_.GetLayer().GetPixelSpacingY();
      originalPanX_ = accessor_.GetLayer().GetPanX();
      originalPanY_ = accessor_.GetLayer().GetPanY();

      switch (corner)
      {
        case Corner_TopLeft:
          oppositeCorner_ = Corner_BottomRight;
          break;

        case Corner_TopRight:
          oppositeCorner_ = Corner_BottomLeft;
          break;

        case Corner_BottomLeft:
          oppositeCorner_ = Corner_TopRight;
          break;

        case Corner_BottomRight:
          oppositeCorner_ = Corner_TopLeft;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      accessor_.GetLayer().GetCorner(oppositeX_, oppositeY_, oppositeCorner_);

      double d = ComputeDistance(x, y, oppositeX_, oppositeY_);
      if (d >= std::numeric_limits<float>::epsilon())
      {
        baseScaling_ = 1.0 / d;
      }
      else
      {
        // Avoid division by zero in extreme cases
        accessor_.Invalidate();
      }
    }
  }


  void RadiographyLayerResizeTracker::Render(CairoContext& context,
                                             double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void RadiographyLayerResizeTracker::MouseUp()
  {
    if (accessor_.IsValid() &&
        accessor_.GetLayer().IsResizeable())
    {
      undoRedoStack_.Add(new UndoRedoCommand(*this));
    }
  }

  
  void RadiographyLayerResizeTracker::MouseMove(int displayX,
                                                int displayY,
                                                double sceneX,
                                                double sceneY)
  {
    static const double ROUND_SCALING = 0.1;
        
    if (accessor_.IsValid() &&
        accessor_.GetLayer().IsResizeable())
    {
      double scaling = ComputeDistance(oppositeX_, oppositeY_, sceneX, sceneY) * baseScaling_;

      if (roundScaling_)
      {
        scaling = boost::math::round<double>((scaling / ROUND_SCALING) * ROUND_SCALING);
      }
          
      RadiographyLayer& layer = accessor_.GetLayer();
      layer.SetPixelSpacing(scaling * originalSpacingX_,
                            scaling * originalSpacingY_);

      // Keep the opposite corner at a fixed location
      double ox, oy;
      layer.GetCorner(ox, oy, oppositeCorner_);
      layer.SetPan(layer.GetPanX() + oppositeX_ - ox,
                   layer.GetPanY() + oppositeY_ - oy);
    }
  }
}
