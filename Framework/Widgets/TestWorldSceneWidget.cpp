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


#include "TestWorldSceneWidget.h"

#include <stdio.h>

namespace OrthancStone
{
  namespace Samples
  {
    class TestWorldSceneWidget::Interactor : public IWorldSceneInteractor
    {
    public:
      virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                          const SliceGeometry& slice,
                                                          const ViewportGeometry& view,
                                                          MouseButton button,
                                                          double x,
                                                          double y,
                                                          IStatusBar* statusBar)
      {
        if (statusBar)
        {
          char buf[64];
          sprintf(buf, "X = %0.2f, Y = %0.2f", x, y);
          statusBar->SetMessage(buf);
        }

        return NULL;
      }

      virtual void MouseOver(CairoContext& context,
                             WorldSceneWidget& widget,
                             const SliceGeometry& slice,
                             const ViewportGeometry& view,
                             double x,
                             double y,
                             IStatusBar* statusBar)
      {
        double S = 0.5;

        if (fabs(x) <= S &&
            fabs(y) <= S)
        {
          cairo_t* cr = context.GetObject();
          cairo_set_source_rgb(cr, 1, 0, 0);
          cairo_rectangle(cr, -S, -S , 2.0 * S, 2.0 * S);
          cairo_set_line_width(cr, 1.0 / view.GetZoom());
          cairo_stroke(cr);
        }
      }

      virtual void MouseWheel(WorldSceneWidget& widget,
                              MouseWheelDirection direction,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
        if (statusBar)
        {
          statusBar->SetMessage(direction == MouseWheelDirection_Down ? "Wheel down" : "Wheel up");
        }
      }

      virtual void KeyPressed(WorldSceneWidget& widget,
                              char key,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
        if (statusBar)
        {
          statusBar->SetMessage("Key pressed: \"" + std::string(1, key) + "\"");
        }
      }
    };


    bool TestWorldSceneWidget::RenderScene(CairoContext& context,
                                           const ViewportGeometry& view)
    {
      cairo_t* cr = context.GetObject();

      // Clear background
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_paint(cr);

      cairo_set_source_rgb(cr, 0, 1, 0);
      cairo_rectangle(cr, -10, -.5, 20, 1);
      cairo_fill(cr);

      return true;
    }


    TestWorldSceneWidget::TestWorldSceneWidget() :
      interactor_(new Interactor)
    {
      SetInteractor(*interactor_);
    }


    void TestWorldSceneWidget::GetSceneExtent(double& x1,
                                              double& y1,
                                              double& x2,
                                              double& y2)
    {
      x1 = -10;
      x2 = 10;
      y1 = -.5;
      y2 = .5;
    }
  }
}
