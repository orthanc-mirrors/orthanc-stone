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

#include "CairoWidget.h"
#include "IWorldSceneInteractor.h"

#include "../Toolbox/ViewportGeometry.h"

namespace Deprecated
{
  class WorldSceneWidget : public CairoWidget
  {
  private:
    class SceneMouseTracker;

    ViewportGeometry       view_;
    IWorldSceneInteractor* interactor_;
    bool                   hasDefaultMouseEvents_;

  protected:
    virtual OrthancStone::Extent2D GetSceneExtent() = 0;

    virtual bool RenderScene(OrthancStone::CairoContext& context,
                             const ViewportGeometry& view) = 0;

    // From CairoWidget
    virtual bool RenderCairo(OrthancStone::CairoContext& context);

    // From CairoWidget
    virtual void RenderMouseOverCairo(OrthancStone::CairoContext& context,
                                      int x,
                                      int y);

    void SetSceneExtent(ViewportGeometry& geometry);

  public:
    WorldSceneWidget(const std::string& name) :
      CairoWidget(name),
      interactor_(NULL),
      hasDefaultMouseEvents_(true)
    {
    }

    void SetDefaultMouseEvents(bool value)
    {
      hasDefaultMouseEvents_ = value;
    }

    bool HasDefaultMouseEvents() const
    {
      return hasDefaultMouseEvents_;
    }

    void SetInteractor(IWorldSceneInteractor& interactor);

    void SetView(const ViewportGeometry& view);

    const ViewportGeometry& GetView() const
    {
      return view_;
    }

    virtual void SetSize(unsigned int width,
                         unsigned int height);

    virtual void FitContent();

    virtual IMouseTracker* CreateMouseTracker(OrthancStone::MouseButton button,
                                              int x,
                                              int y,
                                              OrthancStone::KeyboardModifiers modifiers,
                                              const std::vector<Touch>& touches);

    virtual void MouseWheel(OrthancStone::MouseWheelDirection direction,
                            int x,
                            int y,
                            OrthancStone::KeyboardModifiers modifiers);

    virtual void KeyPressed(OrthancStone::KeyboardKeys key,
                            char keyChar,
                            OrthancStone::KeyboardModifiers modifiers);
  };
}