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

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <vector>

namespace OrthancStone
{
  class Scene2D;
  typedef boost::shared_ptr<Scene2D> Scene2DPtr;
  typedef boost::shared_ptr<const Scene2D> Scene2DConstPtr;

  typedef boost::weak_ptr<Scene2D> Scene2DWPtr;

  class MeasureTool;
  typedef boost::shared_ptr<MeasureTool>
    MeasureToolPtr;
  typedef boost::weak_ptr<MeasureTool>
    MeasureToolWPtr;

  class LineMeasureTool;
  typedef boost::shared_ptr<LineMeasureTool>
    LineMeasureToolPtr;

  class AngleMeasureTool;
  typedef boost::shared_ptr<AngleMeasureTool>
    AngleMeasureToolPtr;

  class IPointerTracker;
  typedef boost::shared_ptr<IPointerTracker>
    PointerTrackerPtr;

  class IFlexiblePointerTracker;
  typedef boost::shared_ptr<IFlexiblePointerTracker>
    FlexiblePointerTrackerPtr;

  typedef boost::shared_ptr<LineMeasureTool>
    LineMeasureToolPtr;

  class CreateMeasureCommand;
  typedef boost::shared_ptr<CreateMeasureCommand>
    CreateMeasureCommandPtr;

  class CreateLineMeasureCommand;
  typedef boost::shared_ptr<CreateLineMeasureCommand>
    CreateLineMeasureCommandPtr;

  class CreateAngleMeasureCommand;
  typedef boost::shared_ptr<CreateAngleMeasureCommand>
    CreateAngleMeasureCommandPtr;

  class TrackerCommand;
  typedef boost::shared_ptr<TrackerCommand> TrackerCommandPtr;

  class ViewportController;
  typedef boost::shared_ptr<ViewportController> ViewportControllerPtr;
  typedef boost::shared_ptr<const ViewportController> ViewportControllerConstPtr;
  typedef boost::weak_ptr<ViewportController> ViewportControllerWPtr;

  class LayerHolder;
  typedef boost::shared_ptr<LayerHolder> LayerHolderPtr;
}
