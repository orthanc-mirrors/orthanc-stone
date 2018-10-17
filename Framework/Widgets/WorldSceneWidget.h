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


#pragma once

#include "CairoWidget.h"
#include "IWorldSceneInteractor.h"

#include "../Toolbox/ObserversRegistry.h"
#include "../Toolbox/ViewportGeometry.h"

namespace OrthancStone
{
  class WorldSceneWidget : public CairoWidget
  {
  public:
    class PanMouseTracker : public IMouseTracker
    {
    private:
      WorldSceneWidget&  that_;
      double             previousPanX_;
      double             previousPanY_;
      double             downX_;
      double             downY_;

    public:
      PanMouseTracker(WorldSceneWidget& that, int x, int y);

      virtual void Render(Orthanc::ImageAccessor& surface) {}

      virtual void MouseUp() {}

      virtual void MouseMove(int x, int y);
    };

    class ZoomMouseTracker : public IMouseTracker
    {
    private:
      WorldSceneWidget&  that_;
      int                downX_;
      int                downY_;
      double             centerX_;
      double             centerY_;
      double             oldZoom_;

    public:
      ZoomMouseTracker(WorldSceneWidget&  that, int x, int y);

      void Render(Orthanc::ImageAccessor& surface) {}

      virtual void MouseUp() {}

      virtual void MouseMove(int x, int y);
    };

  private:
    struct SizeChangeFunctor;

    class SceneMouseTracker;

    ViewportGeometry       view_;
    IWorldSceneInteractor* interactor_;

  public:
    virtual Extent2D GetSceneExtent() = 0;

  protected:
    virtual bool RenderScene(CairoContext& context,
                             const ViewportGeometry& view) = 0;

    virtual bool RenderCairo(CairoContext& context);

    virtual void RenderMouseOverCairo(CairoContext& context,
                                      int x,
                                      int y);

    void SetSceneExtent(ViewportGeometry& geometry);

  public:
    WorldSceneWidget(const std::string& name) :
      CairoWidget(name),
      interactor_(NULL)
    {
    }

    virtual void SetSize(unsigned int width,
                         unsigned int height);

    void SetInteractor(IWorldSceneInteractor& interactor);

    virtual void FitContent();

    void SetView(const ViewportGeometry& view);

    ViewportGeometry GetView();

    virtual IMouseTracker* CreateMouseTracker(MouseButton button,
                                              int x,
                                              int y,
                                              KeyboardModifiers modifiers);

    virtual void RenderSceneMouseOver(CairoContext& context,
                                      const ViewportGeometry& view,
                                      double x,
                                      double y);

    virtual IWorldSceneMouseTracker* CreateMouseSceneTracker(const ViewportGeometry& view,
                                                             MouseButton button,
                                                             double x,
                                                             double y,
                                                             KeyboardModifiers modifiers);

    virtual void MouseWheel(MouseWheelDirection direction,
                            int x,
                            int y,
                            KeyboardModifiers modifiers);

    virtual void KeyPressed(KeyboardKeys key,
                            char keyChar,
                            KeyboardModifiers modifiers);
  };
}
