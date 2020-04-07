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

#include "SampleApplicationBase.h"

#include "../../Framework/Widgets/LayeredSceneWidget.h"
#include "../../Framework/Widgets/IWorldSceneInteractor.h"
#include "../../Framework/Toolbox/ParallelSlicesCursor.h"

namespace OrthancStone
{
  namespace Samples
  {
    /**
     * This is a basic mouse interactor for sample applications. It
     * contains a set of parallel slices in the 3D space. The mouse
     * wheel events make the widget change the slice that is
     * displayed.
     **/
    class SampleInteractor : public IWorldSceneInteractor
    {
    private:
      ParallelSlicesCursor   cursor_;

    public:
      SampleInteractor(VolumeImage& volume,
                       VolumeProjection projection, 
                       bool reverse)
      {
        std::unique_ptr<ParallelSlices> slices(volume.GetGeometry(projection, reverse));
        cursor_.SetGeometry(*slices);
      }

      SampleInteractor(ISeriesLoader& series, 
                       bool reverse)
      {
        if (reverse)
        {
          std::unique_ptr<ParallelSlices> slices(series.GetGeometry().Reverse());
          cursor_.SetGeometry(*slices);
        }
        else
        {
          cursor_.SetGeometry(series.GetGeometry());
        }
      }

      SampleInteractor(const ParallelSlices& slices)
      {
        cursor_.SetGeometry(slices);
      }

      ParallelSlicesCursor& GetCursor()
      {
        return cursor_;
      }

      void AddWidget(LayeredSceneWidget& widget)
      {
        widget.SetInteractor(*this);
        widget.SetSlice(cursor_.GetCurrentSlice());
      }

      virtual IWorldSceneMouseTracker* CreateMouseTracker(WorldSceneWidget& widget,
                                                          const ViewportGeometry& view,
                                                          MouseButton button,
                                                          double x,
                                                          double y,
                                                          IStatusBar* statusBar)
      {
        return NULL;
      }

      virtual void MouseOver(CairoContext& context,
                             WorldSceneWidget& widget,
                             const ViewportGeometry& view,
                             double x,
                             double y,
                             IStatusBar* statusBar)
      {
      }

      virtual void MouseWheel(WorldSceneWidget& widget,
                              MouseWheelDirection direction,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
        if (cursor_.ApplyWheelEvent(direction, modifiers))
        {
          dynamic_cast<LayeredSceneWidget&>(widget).SetSlice(cursor_.GetCurrentSlice());
        }
      }

      virtual void KeyPressed(WorldSceneWidget& widget,
                              char key,
                              KeyboardModifiers modifiers,
                              IStatusBar* statusBar)
      {
      }

      void LookupSliceContainingPoint(LayeredSceneWidget& widget,
                                      const Vector& p)
      {
        if (cursor_.LookupSliceContainingPoint(p))
        {
          widget.SetSlice(cursor_.GetCurrentSlice());
        }
      }
    };
  }
}
