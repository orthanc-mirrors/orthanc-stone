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


#include "../../Framework/Messages/IObserver.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2DViewport/IFlexiblePointerTracker.h"
#include "../../Framework/Scene2DViewport/MeasureTool.h"
#include "../../Framework/Scene2DViewport/PredeclaredTypes.h"
#include "../../Framework/Scene2DViewport/ViewportController.h"
#include "../../Framework/Viewport/SdlViewport.h"

#include <SDL.h>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace OrthancStone
{
  enum GuiTool
  {
    GuiTool_Rotate = 0,
    GuiTool_Pan,
    GuiTool_Zoom,
    GuiTool_LineMeasure,
    GuiTool_CircleMeasure,
    GuiTool_AngleMeasure,
    GuiTool_EllipseMeasure,
    GuiTool_LAST
  };

  const char* MeasureToolToString(size_t i);

  static const unsigned int FONT_SIZE_0 = 32;
  static const unsigned int FONT_SIZE_1 = 24;

  class Scene2D;
  class UndoStack;

  class TrackerSampleApp : public IObserver
    , public boost::enable_shared_from_this<TrackerSampleApp>
  {
  public:
    // 12 because.
    TrackerSampleApp(MessageBroker& broker);
    void PrepareScene();
    void Run();
    void SetInfoDisplayMessage(std::string key, std::string value);
    void DisableTracker();

    void HandleApplicationEvent(const SDL_Event& event);

    /**
    This method is called when the scene transform changes. It allows to
    recompute the visual elements whose content depend upon the scene transform
    */
    void OnSceneTransformChanged(
      const ViewportController::SceneTransformChanged& message);

  private:
    void SelectNextTool();
    void CreateRandomMeasureTool();


    /**
    In the case of this app, the viewport is an SDL viewport and it has 
    a OpenGLCompositor& GetCompositor() method
    */
    ICompositor& GetCompositor();

    /**
    See the other overload
    */
    const ICompositor& GetCompositor() const;

    /**
    This returns a random point in the canvas part of the scene, but in
    scene coordinates
    */
    ScenePoint2D GetRandomPointInScene() const;

    boost::shared_ptr<IFlexiblePointerTracker> TrackerHitTest(const PointerEvent& e);

    boost::shared_ptr<IFlexiblePointerTracker> CreateSuitableTracker(
      const SDL_Event& event,
      const PointerEvent& e);

    void TakeScreenshot(
      const std::string& target,
      unsigned int canvasWidth,
      unsigned int canvasHeight);

    /**
      This adds the command at the top of the undo stack
    */
    void Commit(boost::shared_ptr<TrackerCommand> cmd);
    void Undo();
    void Redo();

  private:
    void DisplayFloatingCtrlInfoText(const PointerEvent& e);
    void DisplayInfoText();
    void HideInfoText();

  private:
    /**
    WARNING: the measuring tools do store a reference to the scene, and it 
    paramount that the scene gets destroyed AFTER the measurement tools.
    */
    boost::shared_ptr<ViewportController> controller_;

    std::map<std::string, std::string> infoTextMap_;
    boost::shared_ptr<IFlexiblePointerTracker> activeTracker_;

    //static const int LAYER_POSITION = 150;

    int TEXTURE_2x2_1_ZINDEX;
    int TEXTURE_1x1_ZINDEX;
    int TEXTURE_2x2_2_ZINDEX;
    int LINESET_1_ZINDEX;
    int LINESET_2_ZINDEX;
    int FLOATING_INFOTEXT_LAYER_ZINDEX;
    int FIXED_INFOTEXT_LAYER_ZINDEX;

    GuiTool currentTool_;
    boost::shared_ptr<UndoStack> undoStack_;
    SdlOpenGLViewport viewport_;
  };

}