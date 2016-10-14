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
