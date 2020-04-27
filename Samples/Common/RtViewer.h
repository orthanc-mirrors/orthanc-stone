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

#include <Framework/Viewport/SdlViewport.h>
#include <Framework/Loaders/GenericLoadersContext.h>
#include <Framework/Messages/IObserver.h>
#include <Framework/Messages/IMessageEmitter.h>
#include <Framework/Oracle/OracleCommandExceptionMessage.h>
#include <Framework/Scene2DViewport/ViewportController.h>
#include <Framework/Volumes/DicomVolumeImage.h>
#include <Framework/Oracle/ThreadedOracle.h>
#include <Framework/Loaders/OrthancSeriesVolumeProgressiveLoader.h>
#include <Framework/Loaders/OrthancMultiframeVolumeLoader.h>
#include <Framework/Loaders/DicomStructureSetLoader.h>

#include <Framework/Messages/ObserverBase.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>

#include <SDL.h>

namespace OrthancStone
{
  class OpenGLCompositor;
  class IVolumeSlicer;
  class ILayerStyleConfigurator;
  class DicomStructureSetLoader;
  class IOracle;
  class ThreadedOracle;
  class VolumeSceneLayerSource;
  class SdlOpenGLViewport;
   
  enum RtViewerGuiTool
  {
    RtViewerGuiTool_Rotate = 0,
    RtViewerGuiTool_Pan,
    RtViewerGuiTool_Zoom,
    RtViewerGuiTool_LineMeasure,
    RtViewerGuiTool_CircleMeasure,
    RtViewerGuiTool_AngleMeasure,
    RtViewerGuiTool_EllipseMeasure,
    RtViewerGuiTool_LAST
  };

  const char* MeasureToolToString(size_t i);

  static const unsigned int FONT_SIZE_0 = 32;
  static const unsigned int FONT_SIZE_1 = 24;

  class Scene2D;
  class UndoStack;

  /**
  This application subclasses IMessageEmitter to use a mutex before forwarding Oracle messages (that
  can be sent from multiple threads)
  */
  class RtViewerApp : public ObserverBase<RtViewerApp>
    , public IMessageEmitter
  {
  public:


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

    /**
    This method will ask the VolumeSceneLayerSource, that are responsible to 
    generated 2D content based on a volume and a cutting plane, to regenerate
    it. This is required if the volume itself changes (during loading) or if 
    the cutting plane is changed
    */
    void UpdateLayers();

    void Refresh();

    virtual void EmitMessage(boost::weak_ptr<IObserver> observer,
      const IMessage& message) ORTHANC_OVERRIDE
    {
      try
      {
        boost::unique_lock<boost::shared_mutex>  lock(mutex_);
        oracleObservable_.EmitMessage(observer, message);
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Exception while emitting a message: " << e.What();
        throw;
      }
    }

    static boost::shared_ptr<RtViewerApp> Create();
    void RegisterMessages();

  protected:
    RtViewerApp();

  private:
#if 1
    // if threaded (not wasm)
    IObservable oracleObservable_;
    ThreadedOracle oracle_;
    boost::shared_mutex mutex_; // to serialize messages from the ThreadedOracle
#endif

    void SelectNextTool();

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
    //void Commit(boost::shared_ptr<TrackerCommand> cmd);
    void Undo();
    void Redo();


    void Handle(const DicomVolumeImage::GeometryReadyMessage& message);
    void Handle(const OracleCommandExceptionMessage& message);
    
    // TODO: wire this
    void HandleCTLoaded(const OrthancSeriesVolumeProgressiveLoader::VolumeImageReadyInHighQuality& message);
    void HandleCTContentUpdated(const OrthancStone::DicomVolumeImage::ContentUpdatedMessage& message);
    void HandleDoseLoaded(const OrthancStone::DicomVolumeImage::ContentUpdatedMessage& message);
    void HandleStructuresReady(const OrthancStone::DicomStructureSetLoader::StructuresReady& message);
    void HandleStructuresUpdated(const OrthancStone::DicomStructureSetLoader::StructuresUpdated& message);

    void SetCtVolume(
      int depth,
      const boost::shared_ptr<IVolumeSlicer>& volume,
      ILayerStyleConfigurator* style);
    
    void SetDoseVolume(
      int depth,
      const boost::shared_ptr<IVolumeSlicer>& volume,
      ILayerStyleConfigurator* style);

    void SetStructureSet(
      int depth, 
      const boost::shared_ptr<DicomStructureSetLoader>& volume);

  private:
    void DisplayFloatingCtrlInfoText(const PointerEvent& e);
    void DisplayInfoText();
    void HideInfoText();
    void RetrieveGeometry();
    void FitContent();

  private:
    boost::shared_ptr<GenericLoadersContext> loadersContext_;
    boost::shared_ptr<VolumeSceneLayerSource>  ctVolumeLayerSource_, doseVolumeLayerSource_, structLayerSource_;
    boost::shared_ptr<OrthancStone::IGeometryProvider> geometryProvider_;
    std::vector<OrthancStone::CoordinateSystem3D>       planes_;
    size_t                                              currentPlane_;

    VolumeProjection                      projection_;

    std::map<std::string, std::string> infoTextMap_;
    boost::shared_ptr<IFlexiblePointerTracker> activeTracker_;

    static const int LAYER_POSITION = 150;

    int TEXTURE_2x2_1_ZINDEX;
    int TEXTURE_1x1_ZINDEX;
    int TEXTURE_2x2_2_ZINDEX;
    int LINESET_1_ZINDEX;
    int LINESET_2_ZINDEX;
    int FLOATING_INFOTEXT_LAYER_ZINDEX;
    int FIXED_INFOTEXT_LAYER_ZINDEX;

    RtViewerGuiTool currentTool_;
    boost::shared_ptr<UndoStack> undoStack_;
    boost::shared_ptr<SdlOpenGLViewport> viewport_;
  };

}


 