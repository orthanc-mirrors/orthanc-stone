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


#include "PanMouseTracker.h"

#include <Logging.h>
#include <OrthancException.h>

namespace Deprecated
{
  PanMouseTracker::PanMouseTracker(WorldSceneWidget& that,
                                   int x,
                                   int y) :
    that_(that)
  {
    that.GetView().GetPan(originalPanX_, originalPanY_);
    that.GetView().MapPixelCenterToScene(downX_, downY_, x, y);
  }
    

  void PanMouseTracker::Render(OrthancStone::CairoContext& context,
                               double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }


  void PanMouseTracker::MouseMove(int displayX,
                                  int displayY,
                                  double x,
                                  double y,
                                  const std::vector<Touch>& displayTouches,
                                  const std::vector<Touch>& sceneTouches)
  {
    ViewportGeometry view = that_.GetView();
    view.SetPan(originalPanX_ + (x - downX_) * view.GetZoom(),
                originalPanY_ + (y - downY_) * view.GetZoom());
    that_.SetView(view);
  }
}