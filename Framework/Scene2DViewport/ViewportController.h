/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "PointerTypes.h"

#include "../Scene2D/Scene2D.h"
#include "../Scene2D/PointerEvent.h"
#include "../Scene2DViewport/IFlexiblePointerTracker.h"

#include <stack>

namespace OrthancStone
{
  /**
    These constats are used 
  
  */
  const double ARC_RADIUS_CANVAS_COORD = 30.0;
  const double TEXT_CENTER_DISTANCE_CANVAS_COORD = 90;

  const double HANDLE_SIDE_LENGTH_CANVAS_COORD = 10.0;
  const double HIT_TEST_MAX_DISTANCE_CANVAS_COORD = 15.0;

  const uint8_t TEXT_COLOR_RED = 0;
  const uint8_t TEXT_COLOR_GREEN = 223;
  const uint8_t TEXT_COLOR_BLUE = 81;

  const uint8_t TOOL_LINES_COLOR_RED = 0;
  const uint8_t TOOL_LINES_COLOR_GREEN = 223;
  const uint8_t TOOL_LINES_COLOR_BLUE = 21;


  const uint8_t TEXT_OUTLINE_COLOR_RED = 0;
  const uint8_t TEXT_OUTLINE_COLOR_GREEN = 56;
  const uint8_t TEXT_OUTLINE_COLOR_BLUE = 21;

  /**
  This object is responsible for hosting a scene, responding to messages from
  the model and updating the scene accordingly.

  It contains the list of active measuring tools as well as the stack
  where measuring tool commands are stored.

  The active tracker is also stored in the viewport controller.

  Each canvas or other GUI area where we want to display a 2D image, either 
  directly or through slicing must be assigned a ViewportController.
  */
  class ViewportController : public IObservable
  {
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, \
      SceneTransformChanged, ViewportController);

    ViewportController(MessageBroker& broker);

    Scene2DConstPtr GetScene() const;
    Scene2DPtr      GetScene();

    /** 
    This method is called by the GUI system and should update/delete the
    current tracker
    */
    bool HandlePointerEvent(PointerEvent e);

    /**
    This method returns the list of measure tools containing the supplied point
    (in scene coords). A tracker can then be requested from the chosen 
    measure tool, if needed
    */
    std::vector<MeasureToolPtr> HitTestMeasureTools(ScenePoint2D p);

    /**
    With this method, the object takes ownership of the supplied tracker and
    updates it according to user interaction
    */
    void SetActiveTracker(FlexiblePointerTrackerPtr tracker);

    /** Forwarded to the underlying scene */
    const AffineTransform2D& GetCanvasToSceneTransform() const;

    /** Forwarded to the underlying scene */
    const AffineTransform2D& GetSceneToCanvasTransform() const;

    /** Forwarded to the underlying scene, and broadcasted to the observers */
    void SetSceneToCanvasTransform(const AffineTransform2D& transform);

    /** Forwarded to the underlying scene, and broadcasted to the observers */
    void FitContent(unsigned int canvasWidth, unsigned int canvasHeight);

    /** 
    Stores a command : 
    - this first trims the undo stack to keep the first numAppliedCommands_ 
    - then it adds the supplied command at the top of the undo stack

    In other words, when a new command is pushed, all the undone (and not 
    redone) commands are removed.
    */
    void PushCommand(TrackerCommandPtr command);

    /**
    Undoes the command at the top of the undo stack, or throws if there is no
    command to undo.
    You can check "CanUndo" first to protect against extraneous redo.
    */
    void Undo();

    /**
    Redoes the command that is just above the last applied command in the undo
    stack or throws if there is no command to redo. 
    You can check "CanRedo" first to protect against extraneous redo.
    */
    void Redo();

    /** selfexpl */
    bool CanUndo() const;

    /** selfexpl */
    bool CanRedo() const;

    /** Adds a new measure tool */
    void AddMeasureTool(MeasureToolPtr measureTool);

    /** Removes a measure tool or throws if it cannot be found */
    void RemoveMeasureTool(MeasureToolPtr measureTool);

    /**
    The square handle side length in *scene* coordinates
    */
    double GetHandleSideLengthS() const;

    /**
    The angle measure too arc  radius in *scene* coordinates

    Note: you might wonder why this is not part of the AngleMeasureTool itself,
    but we prefer to put all such constants in the same location, to ease 
    */
    double GetAngleToolArcRadiusS() const;

    /**
    The hit test maximum distance in *scene* coordinates.
    If a pointer event is less than GetHandleSideLengthS() to a GUI element,
    the hit test for this GUI element is seen as true
    */
    double GetHitTestMaximumDistanceS() const;

    /**
    Distance between the top of the angle measuring tool and the center of 
    the label showing the actual measure, in *scene* coordinates
    */
    double GetAngleTopTextLabelDistanceS() const;

  private:
    double GetCanvasToSceneFactor() const;

    std::vector<TrackerCommandPtr> commandStack_;
    
    /**
    This is always between >= 0 and <= undoStack_.size() and gives the 
    position where the controller is in the undo stack. 
    - If numAppliedCommands_ > 0, one can undo
    - If numAppliedCommands_ < numAppliedCommands_.size(), one can redo
    */
    size_t                      numAppliedCommands_;
    std::vector<MeasureToolPtr> measureTools_;
    Scene2DPtr                  scene_;
    FlexiblePointerTrackerPtr   tracker_;
    
    // this is cached
    mutable double              canvasToSceneFactor_;
  };
}
