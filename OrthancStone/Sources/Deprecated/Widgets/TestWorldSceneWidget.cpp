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


#include "TestWorldSceneWidget.h"

#include <OrthancException.h>

#include <math.h>
#include <stdio.h>

namespace Deprecated
{
  namespace Samples
  {
    class TestWorldSceneWidget::Interactor : public IWorldSceneInteractor
    {
    public:
      virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                          const ViewportGeometry& view,
                                                          OrthancStone::MouseButton button,
                                                          OrthancStone::KeyboardModifiers modifiers,
                                                          int viewportX,
                                                          int viewportY,
                                                          double x,
                                                          double y,
                                                          IStatusBar* statusBar,
                                                          const std::vector<Touch>& touches)
      {
        if (statusBar)
        {
          char buf[64];
          sprintf(buf, "X = %0.2f, Y = %0.2f", x, y);
          statusBar->SetMessage(buf);
        }

        return NULL;
      }

      virtual void MouseOver(OrthancStone::CairoContext& context,
                             WorldSceneWidget& widget,
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
                              OrthancStone::MouseWheelDirection direction,
                              OrthancStone::KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
        if (statusBar)
        {
          statusBar->SetMessage(direction == OrthancStone::MouseWheelDirection_Down ? "Wheel down" : "Wheel up");
        }
      }

      virtual void KeyPressed(WorldSceneWidget& widget,
                              OrthancStone::KeyboardKeys key,
                              char keyChar,
                              OrthancStone::KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
        if (statusBar)
        {
          statusBar->SetMessage("Key pressed: \"" + std::string(1, keyChar) + "\"");
        }
      }
    };


    bool TestWorldSceneWidget::RenderScene(OrthancStone::CairoContext& context,
                                           const ViewportGeometry& view)
    {
      cairo_t* cr = context.GetObject();

      // Clear background
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_paint(cr);

      float color = static_cast<float>(count_ % 16) / 15.0f;
      cairo_set_source_rgb(cr, 0, 1.0f - color, color);
      cairo_rectangle(cr, -10, -.5, 20, 1);
      cairo_fill(cr);

      return true;
    }


    TestWorldSceneWidget::TestWorldSceneWidget(const std::string& name, bool animate) :
      WorldSceneWidget(name),
      interactor_(new Interactor),
      animate_(animate),
      count_(0)
    {
      SetInteractor(*interactor_);
    }


    OrthancStone::Extent2D TestWorldSceneWidget::GetSceneExtent()
    {
      return OrthancStone::Extent2D(-10, -.5, 10, .5);
    }


    void TestWorldSceneWidget::DoAnimation()
    {
      if (animate_)
      {
        count_++;
        NotifyContentChanged();
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
  }
}