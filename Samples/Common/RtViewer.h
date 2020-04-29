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

#include <Framework/Viewport/IViewport.h>

#include <Framework/Loaders/DicomStructureSetLoader.h>
#include <Framework/Loaders/OrthancMultiframeVolumeLoader.h>
#include <Framework/Loaders/OrthancSeriesVolumeProgressiveLoader.h>
#include <Framework/Loaders/ILoadersContext.h>
#include <Framework/Messages/IMessageEmitter.h>
#include <Framework/Messages/IObserver.h>
#include <Framework/Messages/ObserverBase.h>
#include <Framework/Oracle/OracleCommandExceptionMessage.h>
#include <Framework/Scene2DViewport/ViewportController.h>
#include <Framework/Volumes/DicomVolumeImage.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>

#if ORTHANC_ENABLE_SDL
#include <SDL.h>
#endif

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
  {
  public:

    void PrepareScene();

#if ORTHANC_ENABLE_SDL
  public:
    void RunSdl(int argc, char* argv[]);
  private:
    void ProcessOptions(int argc, char* argv[]);
    void HandleApplicationEvent(const SDL_Event& event);
#elif ORTHANC_ENABLE_WASM
  public:
    void RunWasm();
#else
#  error Either ORTHANC_ENABLE_SDL or ORTHANC_ENABLE_WASM must be enabled
#endif

  public:
    void SetInfoDisplayMessage(std::string key, std::string value);
    void DisableTracker();

    /**
    Called by command-line option processing or when parsing the URL 
    parameters.
    */
    void SetArgument(const std::string& key, const std::string& value);

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

#if 0
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
#endif

    static boost::shared_ptr<RtViewerApp> Create();
    void RegisterMessages();

  protected:
    RtViewerApp();

  private:
    void PrepareLoadersAndSlicers();
    void SelectNextTool();

    // argument handling
    // SetArgument is above (public section)
    std::map<std::string, std::string> arguments_;

    const std::string& GetArgument(const std::string& key) const;
    bool HasArgument(const std::string& key) const;

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


    void HandleGeometryReady(const DicomVolumeImage::GeometryReadyMessage& message);
    
    // TODO: wire this
    void HandleCTLoaded(const OrthancSeriesVolumeProgressiveLoader::VolumeImageReadyInHighQuality& message);
    void HandleCTContentUpdated(const OrthancStone::DicomVolumeImage::ContentUpdatedMessage& message);
    void HandleDoseLoaded(const OrthancStone::DicomVolumeImage::ContentUpdatedMessage& message);
    void HandleStructuresReady(const OrthancStone::DicomStructureSetLoader::StructuresReady& message);
    void HandleStructuresUpdated(const OrthancStone::DicomStructureSetLoader::StructuresUpdated& message);

    void SetCtVolumeSlicer(
      int depth,
      const boost::shared_ptr<IVolumeSlicer>& volume,
      ILayerStyleConfigurator* style);
    
    void SetDoseVolumeSlicer(
      int depth,
      const boost::shared_ptr<IVolumeSlicer>& volume,
      ILayerStyleConfigurator* style);

    void SetStructureSet(
      int depth, 
      const boost::shared_ptr<DicomStructureSetLoader>& volume);

  private:
    void CreateViewport();
    void DisplayFloatingCtrlInfoText(const PointerEvent& e);
    void DisplayInfoText();
    void HideInfoText();
    void RetrieveGeometry();
    void FitContent();

  private:
    boost::shared_ptr<DicomVolumeImage>  ctVolume_;
    boost::shared_ptr<DicomVolumeImage>  doseVolume_;

    boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader> ctLoader_;
    boost::shared_ptr<OrthancMultiframeVolumeLoader> doseLoader_;
    boost::shared_ptr<DicomStructureSetLoader>  rtstructLoader_;

    /** encapsulates resources shared by loaders */
    boost::shared_ptr<ILoadersContext>                  loadersContext_;

    boost::shared_ptr<VolumeSceneLayerSource>           ctVolumeLayerSource_, doseVolumeLayerSource_, structLayerSource_;
    
    /**
    another interface to the ctLoader object (that also implements the IVolumeSlicer interface), that serves as the 
    reference for the geometry (position and dimensions of the volume + size of each voxel). It could be changed to be 
    the dose instead, but the CT is chosen because it usually has a better spatial resolution.
    */
    boost::shared_ptr<OrthancStone::IGeometryProvider>  geometryProvider_;

    // collection of cutting planes for this particular view
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
    boost::shared_ptr<IViewport> viewport_;
  };

}


 