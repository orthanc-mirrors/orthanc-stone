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


#pragma once

#include "../../../../Framework/Deprecated/Widgets/IWorldSceneInteractor.h"

using namespace OrthancStone;

namespace SimpleViewer {

  class SimpleViewerApplication;

  class ThumbnailInteractor : public Deprecated::IWorldSceneInteractor
  {
  private:
    SimpleViewerApplication&  application_;
  public:
    ThumbnailInteractor(SimpleViewerApplication&  application) :
      application_(application)
    {
    }

    virtual Deprecated::IWorldSceneMouseTracker* CreateMouseTracker(Deprecated::WorldSceneWidget& widget,
                                                                    const Deprecated::ViewportGeometry& view,
                                                                    MouseButton button,
                                                                    KeyboardModifiers modifiers,
                                                                    int viewportX,
                                                                    int viewportY,
                                                                    double x,
                                                                    double y,
                                                                    Deprecated::IStatusBar* statusBar,
                                                                    const std::vector<Deprecated::Touch>& displayTouches);

    virtual void MouseOver(CairoContext& context,
                           Deprecated::WorldSceneWidget& widget,
                           const Deprecated::ViewportGeometry& view,
                           double x,
                           double y,
                           Deprecated::IStatusBar* statusBar)
    {}

    virtual void MouseWheel(Deprecated::WorldSceneWidget& widget,
                            MouseWheelDirection direction,
                            KeyboardModifiers modifiers,
                            Deprecated::IStatusBar* statusBar)
    {}

    virtual void KeyPressed(Deprecated::WorldSceneWidget& widget,
                            KeyboardKeys key,
                            char keyChar,
                            KeyboardModifiers modifiers,
                            Deprecated::IStatusBar* statusBar)
    {}

  };


}