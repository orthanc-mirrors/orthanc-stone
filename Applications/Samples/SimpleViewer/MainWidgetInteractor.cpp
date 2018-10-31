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

#include "MainWidgetInteractor.h"

#include "SimpleViewerApplication.h"

namespace SimpleViewer {

  IWorldSceneMouseTracker* MainWidgetInteractor::CreateMouseTracker(WorldSceneWidget& widget,
                                                                    const ViewportGeometry& view,
                                                                    MouseButton button,
                                                                    KeyboardModifiers modifiers,
                                                                    int viewportX,
                                                                    int viewportY,
                                                                    double x,
                                                                    double y,
                                                                    IStatusBar* statusBar)
  {
    if (button == MouseButton_Left)
    {
      if (application_.GetCurrentTool() == SimpleViewerApplication::Tools_LineMeasure)
      {
        return new LineMeasureTracker(statusBar, dynamic_cast<LayerWidget&>(widget).GetSlice(), x, y, 255, 0, 0, 10);
      }
      else if (application_.GetCurrentTool() == SimpleViewerApplication::Tools_CircleMeasure)
      {
        return new CircleMeasureTracker(statusBar, dynamic_cast<LayerWidget&>(widget).GetSlice(), x, y, 255, 0, 0, 10);
      }
      else if (application_.GetCurrentTool() == SimpleViewerApplication::Tools_Crop)
      {
        // TODO
      }
      else if (application_.GetCurrentTool() == SimpleViewerApplication::Tools_Windowing)
      {
        // TODO
      }
      else if (application_.GetCurrentTool() == SimpleViewerApplication::Tools_Zoom)
      {
        // TODO
      }
      else if (application_.GetCurrentTool() == SimpleViewerApplication::Tools_Pan)
      {
        // TODO
      }

    }
    return NULL;
  }

  void MainWidgetInteractor::MouseOver(CairoContext& context,
                                       WorldSceneWidget& widget,
                                       const ViewportGeometry& view,
                                       double x,
                                       double y,
                                       IStatusBar* statusBar)
  {
    if (statusBar != NULL)
    {
      Vector p = dynamic_cast<LayerWidget&>(widget).GetSlice().MapSliceToWorldCoordinates(x, y);

      char buf[64];
      sprintf(buf, "X = %.02f Y = %.02f Z = %.02f (in cm)",
              p[0] / 10.0, p[1] / 10.0, p[2] / 10.0);
      statusBar->SetMessage(buf);
    }
  }

  void MainWidgetInteractor::MouseWheel(WorldSceneWidget& widget,
                                        MouseWheelDirection direction,
                                        KeyboardModifiers modifiers,
                                        IStatusBar* statusBar)
  {
  }

  void MainWidgetInteractor::KeyPressed(WorldSceneWidget& widget,
                                        KeyboardKeys key,
                                        char keyChar,
                                        KeyboardModifiers modifiers,
                                        IStatusBar* statusBar)
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
