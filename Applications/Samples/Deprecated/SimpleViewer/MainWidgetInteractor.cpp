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

#include "MainWidgetInteractor.h"

#include "SimpleViewerApplication.h"

namespace SimpleViewer {

  Deprecated::IWorldSceneMouseTracker* MainWidgetInteractor::CreateMouseTracker(Deprecated::WorldSceneWidget& widget,
                                                                    const Deprecated::ViewportGeometry& view,
                                                                    MouseButton button,
                                                                    KeyboardModifiers modifiers,
                                                                    int viewportX,
                                                                    int viewportY,
                                                                    double x,
                                                                    double y,
                                                                    Deprecated::IStatusBar* statusBar,
                                                                    const std::vector<Deprecated::Touch>& displayTouches)
  {
    if (button == MouseButton_Left)
    {
      if (application_.GetCurrentTool() == Tool_LineMeasure)
      {
        return new Deprecated::LineMeasureTracker(statusBar, dynamic_cast<Deprecated::SliceViewerWidget&>(widget).GetSlice(),
                                      x, y, 255, 0, 0, application_.GetFont());
      }
      else if (application_.GetCurrentTool() == Tool_CircleMeasure)
      {
        return new Deprecated::CircleMeasureTracker(statusBar, dynamic_cast<Deprecated::SliceViewerWidget&>(widget).GetSlice(),
                                        x, y, 255, 0, 0, application_.GetFont());
      }
      else if (application_.GetCurrentTool() == Tool_Crop)
      {
        // TODO
      }
      else if (application_.GetCurrentTool() == Tool_Windowing)
      {
        // TODO
      }
      else if (application_.GetCurrentTool() == Tool_Zoom)
      {
        // TODO
      }
      else if (application_.GetCurrentTool() == Tool_Pan)
      {
        // TODO
      }
    }
    return NULL;
  }

  void MainWidgetInteractor::MouseOver(CairoContext& context,
                                       Deprecated::WorldSceneWidget& widget,
                                       const Deprecated::ViewportGeometry& view,
                                       double x,
                                       double y,
                                       Deprecated::IStatusBar* statusBar)
  {
    if (statusBar != NULL)
    {
      Vector p = dynamic_cast<Deprecated::SliceViewerWidget&>(widget).GetSlice().MapSliceToWorldCoordinates(x, y);

      char buf[64];
      sprintf(buf, "X = %.02f Y = %.02f Z = %.02f (in cm)",
              p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
      statusBar->SetMessage(buf);
    }
  }

  void MainWidgetInteractor::MouseWheel(Deprecated::WorldSceneWidget& widget,
                                        MouseWheelDirection direction,
                                        KeyboardModifiers modifiers,
                                        Deprecated::IStatusBar* statusBar)
  {
  }

  void MainWidgetInteractor::KeyPressed(Deprecated::WorldSceneWidget& widget,
                                        KeyboardKeys key,
                                        char keyChar,
                                        KeyboardModifiers modifiers,
                                        Deprecated::IStatusBar* statusBar)
  {
    switch (keyChar)
    {
    case 's':
      widget.FitContent();
      break;

    default:
      break;
    }
  }
}
