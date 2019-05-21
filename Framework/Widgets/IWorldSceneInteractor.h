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


#pragma once

#include "IWorldSceneMouseTracker.h"

#include "../Toolbox/ViewportGeometry.h"
#include "../StoneEnumerations.h"
#include "../Viewport/IStatusBar.h"

namespace Deprecated
{
    class WorldSceneWidget;

    class IWorldSceneInteractor : public boost::noncopyable
    {
    public:
        virtual ~IWorldSceneInteractor()
        {
        }

        virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                            const ViewportGeometry& view,
                                                            OrthancStone::MouseButton button,
                                                            OrthancStone::KeyboardModifiers modifiers,
                                                            int viewportX,
                                                            int viewportY,
                                                            double x,
                                                            double y,
                                                            IStatusBar* statusBar,
                                                            const std::vector<Touch>& touches) = 0;

        virtual void MouseOver(OrthancStone::CairoContext& context,
                               WorldSceneWidget& widget,
                               const ViewportGeometry& view,
                               double x,
                               double y,
                               IStatusBar* statusBar) = 0;

        virtual void MouseWheel(WorldSceneWidget& widget,
                                OrthancStone::MouseWheelDirection direction,
                                OrthancStone::KeyboardModifiers modifiers,
                                IStatusBar* statusBar) = 0;

        virtual void KeyPressed(WorldSceneWidget& widget,
                                OrthancStone::KeyboardKeys key,
                                char keyChar,
                                OrthancStone::KeyboardModifiers modifiers,
                                IStatusBar* statusBar) = 0;
    };
}
