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

#include <Framework/Scene2D/OpenGLCompositor.h>
#include <Framework/Messages/IObserver.h>

#include "../Common/IFlexiblePointerTracker.h"
#include "../Common/MeasureTools.h"

#include <SDL.h>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace OrthancStone
{
  class TrackerCommand;
  typedef boost::shared_ptr<TrackerCommand> TrackerCommandPtr;

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

    Scene2D& GetScene();

    void HandleApplicationEvent(const SDL_Event& event);

    /**
    This method is called when the scene transform changes. It allows to
    recompute the visual elements whose content depend upon the scene transform
    */
    void OnSceneTransformChanged(const Scene2D::SceneTransformChanged& message);

  private:
    void SelectNextTool();


    FlexiblePointerTrackerPtr TrackerHitTest(const PointerEvent& e);

    FlexiblePointerTrackerPtr CreateSuitableTracker(
      const SDL_Event& event,
      const PointerEvent& e);

    void TakeScreenshot(
      const std::string& target,
      unsigned int canvasWidth,
      unsigned int canvasHeight);

    /**
      This adds the command at the top of the undo stack
    */
    void Commit(TrackerCommandPtr cmd);
    void Undo();
    void Redo();

  private:
    void DisplayFloatingCtrlInfoText(const PointerEvent& e);
    void DisplayInfoText();
    void HideInfoText();

  private:
    std::auto_ptr<OpenGLCompositor> compositor_;
    /**
    WARNING: the measuring tools do store a reference to the scene, and it 
    paramount that the scene gets destroyed AFTER the measurement tools.
    */
    Scene2D scene_;

    std::map<std::string, std::string> infoTextMap_;
    FlexiblePointerTrackerPtr activeTracker_;
    std::vector<TrackerCommandPtr> undoStack_;

    // we store the measure tools here so that they don't get deleted
    std::vector<MeasureToolPtr> measureTools_;

    //static const int LAYER_POSITION = 150;


    int TEXTURE_2x2_1_ZINDEX;
    int TEXTURE_1x1_ZINDEX;
    int TEXTURE_2x2_2_ZINDEX;
    int LINESET_1_ZINDEX;
    int LINESET_2_ZINDEX;
    int FLOATING_INFOTEXT_LAYER_ZINDEX;
    int FIXED_INFOTEXT_LAYER_ZINDEX;

    GuiTool currentTool_;
  };

}
