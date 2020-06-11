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


#include "RadiographyWindowingTracker.h"
#include "RadiographyWidget.h"

#include <OrthancException.h>


namespace OrthancStone
{
  class RadiographyWindowingTracker::UndoRedoCommand : public UndoRedoStack::ICommand
  {
  private:
    RadiographyScene&  scene_;
    float              sourceCenter_;
    float              sourceWidth_;
    float              targetCenter_;
    float              targetWidth_;

  public:
    UndoRedoCommand(const RadiographyWindowingTracker& tracker) :
      scene_(tracker.scene_),
      sourceCenter_(tracker.sourceCenter_),
      sourceWidth_(tracker.sourceWidth_)
    {
      scene_.GetWindowingWithDefault(targetCenter_, targetWidth_);
    }

    virtual void Undo() const
    {
      scene_.SetWindowing(sourceCenter_, sourceWidth_);
    }
      
    virtual void Redo() const
    {
      scene_.SetWindowing(targetCenter_, targetWidth_);
    }
  };


  void RadiographyWindowingTracker::ComputeAxisEffect(int& deltaCenter,
                                                      int& deltaWidth,
                                                      int delta,
                                                      Action actionNegative,
                                                      Action actionPositive)
  {
    if (delta < 0)
    {
      switch (actionNegative)
      {
        case Action_IncreaseWidth:
          deltaWidth = -delta;
          break;

        case Action_DecreaseWidth:
          deltaWidth = delta;
          break;

        case Action_IncreaseCenter:
          deltaCenter = -delta;
          break;

        case Action_DecreaseCenter:
          deltaCenter = delta;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
    else if (delta > 0)
    {
      switch (actionPositive)
      {
        case Action_IncreaseWidth:
          deltaWidth = delta;
          break;

        case Action_DecreaseWidth:
          deltaWidth = -delta;
          break;

        case Action_IncreaseCenter:
          deltaCenter = delta;
          break;

        case Action_DecreaseCenter:
          deltaCenter = -delta;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  }
    
    
  RadiographyWindowingTracker::RadiographyWindowingTracker(UndoRedoStack& undoRedoStack,
                                                           RadiographyScene& scene,
                                                           RadiographyWidget& widget,
                                                           ImageInterpolation interpolationDuringTracking,
                                                           int x,
                                                           int y,
                                                           Action leftAction,
                                                           Action rightAction,
                                                           Action upAction,
                                                           Action downAction) :
    undoRedoStack_(undoRedoStack),
    scene_(scene),
    widget_(widget),
    initialWidgetInterpolation_(widget.GetInterpolation()),
    clickX_(x),
    clickY_(y),
    leftAction_(leftAction),
    rightAction_(rightAction),
    upAction_(upAction),
    downAction_(downAction)
  {
    scene_.GetWindowingWithDefault(sourceCenter_, sourceWidth_);
    widget_.SetInterpolation(interpolationDuringTracking);

    float minValue, maxValue;
    scene.GetRange(minValue, maxValue);

    assert(minValue <= maxValue);

    float delta = (maxValue - minValue);
    strength_ = delta / 1000.0f; // 1px move will change the ww/wc by 0.1%

    if (strength_ < 1)
    {
      strength_ = 1;
    }
  }


  void RadiographyWindowingTracker::Render(CairoContext& context,
                                           double zoom)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
  

  void RadiographyWindowingTracker::MouseUp()
  {
    widget_.SetInterpolation(initialWidgetInterpolation_);
    undoRedoStack_.Add(new UndoRedoCommand(*this));
  }


  void RadiographyWindowingTracker::MouseMove(int displayX,
                                              int displayY,
                                              double sceneX,
                                              double sceneY,
                                              const std::vector<Deprecated::Touch>& displayTouches,
                                              const std::vector<Deprecated::Touch>& sceneTouches)
  {
    // This follows the behavior of the Osimis Web viewer:
    // https://bitbucket.org/osimis/osimis-webviewer-plugin/src/master/frontend/src/app/viewport/image-plugins/windowing-viewport-tool.class.js

    static const float SCALE = 1.0;
      
    int deltaCenter = 0;
    int deltaWidth = 0;

    ComputeAxisEffect(deltaCenter, deltaWidth, displayX - clickX_, leftAction_, rightAction_);
    ComputeAxisEffect(deltaCenter, deltaWidth, displayY - clickY_, upAction_, downAction_);

    float newCenter = sourceCenter_ + (deltaCenter / SCALE * strength_);
    float newWidth = sourceWidth_ + (deltaWidth / SCALE * strength_);
    scene_.SetWindowing(newCenter, newWidth);
  }
}
