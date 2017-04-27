/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "IWorldSceneMouseTracker.h"

#include "../Toolbox/SliceGeometry.h"
#include "../Toolbox/ViewportGeometry.h"
#include "../Enumerations.h"
#include "../Viewport/IStatusBar.h"

namespace OrthancStone
{
  class WorldSceneWidget;

  class IWorldSceneInteractor : public IThreadSafe
  {
  public:
    virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                        const SliceGeometry& slice,
                                                        const ViewportGeometry& view,
                                                        MouseButton button,
                                                        double x,
                                                        double y,
                                                        IStatusBar* statusBar) = 0;

    virtual void MouseOver(CairoContext& context,
                           WorldSceneWidget& widget,
                           const SliceGeometry& slice,
                           const ViewportGeometry& view,
                           double x,
                           double y,
                           IStatusBar* statusBar) = 0;

    virtual void MouseWheel(WorldSceneWidget& widget,
                            MouseWheelDirection direction,
                            KeyboardModifiers modifiers,
                            IStatusBar* statusBar) = 0;

    virtual void KeyPressed(WorldSceneWidget& widget,
                            char key,
                            KeyboardModifiers modifiers,
                            IStatusBar* statusBar) = 0;
  };
}
