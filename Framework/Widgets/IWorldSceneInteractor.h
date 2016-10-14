/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
