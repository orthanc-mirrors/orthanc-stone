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


#include "RadiographyLayerRotateTracker.h"

#include "RadiographySceneCommand.h"

#include <OrthancException.h>

#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/round.hpp>

namespace OrthancStone
{
  class RadiographyLayerRotateTracker::UndoRedoCommand : public RadiographySceneCommand
  {
  private:
    double  sourceAngle_;
    double  targetAngle_;

    static int ToDegrees(double angle)
    {
      return boost::math::iround(angle * 180.0 / boost::math::constants::pi<double>());
    }
      
  protected:
    virtual void UndoInternal(RadiographyLayer& layer) const
    {
      LOG(INFO) << "Undo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
      layer.SetAngle(sourceAngle_);
    }

    virtual void RedoInternal(RadiographyLayer& layer) const
    {
      LOG(INFO) << "Redo - Set angle to " << ToDegrees(sourceAngle_) << " degrees";
      layer.SetAngle(targetAngle_);
    }

  public:
    UndoRedoCommand(const RadiographyLayerRotateTracker& tracker) :
      RadiographySceneCommand(tracker.accessor_),
      sourceAngle_(tracker.originalAngle_),
      targetAngle_(tracker.accessor_.GetLayer().GetGeometry().GetAngle())
    {
    }
  };


  bool RadiographyLayerRotateTracker::ComputeAngle(double& angle /* out */,
                                                   double sceneX,
                                                   double sceneY) const
  {
    Vector u;
    LinearAlgebra::AssignVector(u, sceneX - centerX_, sceneY - centerY_);

    double nu = boost::numeric::ublas::norm_2(u);

    if (!LinearAlgebra::IsCloseToZero(nu))
    {
      u /= nu;
      angle = atan2(u[1], u[0]);
      return true;
    }
    else
    {
      return false;
    }
  }


  RadiographyLayerRotateTracker::RadiographyLayerRotateTracker(UndoRedoStack& undoRedoStack,
                                                               RadiographyScene& scene,
                                                               const Deprecated::ViewportGeometry& view,
                                                               size_t layer,
                                                               double x,
                                                               double y,
                                                               bool roundAngles) :
    undoRedoStack_(undoRedoStack),
    accessor_(scene, layer),
    roundAngles_(roundAngles)
  {
    if (accessor_.IsValid())
    {
      accessor_.GetLayer().GetCenter(centerX_, centerY_);
      originalAngle_ = accessor_.GetLayer().GetGeometry().GetAngle();

      double sceneX, sceneY;
      view.MapDisplayToScene(sceneX, sceneY, x, y);

      if (!ComputeAngle(clickAngle_, x, y))
      {
        accessor_.Invalidate();
      }
    }
  }


  void RadiographyLayerRotateTracker::Render(CairoContext& context,
                                             double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void RadiographyLayerRotateTracker::MouseUp()
  {
    if (accessor_.IsValid())
    {
      undoRedoStack_.Add(new UndoRedoCommand(*this));
    }
  }

  
  void RadiographyLayerRotateTracker::MouseMove(int displayX,
                                                int displayY,
                                                double sceneX,
                                                double sceneY,
                                                const std::vector<Deprecated::Touch>& displayTouches,
                                                const std::vector<Deprecated::Touch>& sceneTouches)
  {
    static const double ROUND_ANGLE = 15.0 / 180.0 * boost::math::constants::pi<double>(); 
        
    double angle;
        
    if (accessor_.IsValid() &&
        ComputeAngle(angle, sceneX, sceneY))
    {
      angle = angle - clickAngle_ + originalAngle_;

      if (roundAngles_)
      {
        angle = boost::math::round<double>((angle / ROUND_ANGLE) * ROUND_ANGLE);
      }
          
      accessor_.GetLayer().SetAngle(angle);
    }
  }
}
