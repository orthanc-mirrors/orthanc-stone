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

#include "RtViewer.h"

#include <stdio.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <Framework/OpenGL/SdlOpenGLContext.h>
#include <Framework/StoneInitialization.h>

#include <Framework/Scene2D/CairoCompositor.h>
#include <Framework/Scene2D/ColorTextureSceneLayer.h>
#include <Framework/Scene2D/OpenGLCompositor.h>
#include <Framework/Scene2D/PanSceneTracker.h>
#include <Framework/Scene2D/ZoomSceneTracker.h>
#include <Framework/Scene2D/RotateSceneTracker.h>

#include <Framework/Scene2DViewport/UndoStack.h>
#include <Framework/Scene2DViewport/CreateLineMeasureTracker.h>
#include <Framework/Scene2DViewport/CreateAngleMeasureTracker.h>
#include <Framework/Scene2DViewport/IFlexiblePointerTracker.h>
#include <Framework/Scene2DViewport/MeasureTool.h>
#include <Framework/Scene2DViewport/PredeclaredTypes.h>
#include <Framework/Volumes/VolumeSceneLayerSource.h>

#include <Framework/Oracle/GetOrthancWebViewerJpegCommand.h>
#include <Framework/Oracle/ThreadedOracle.h>
#include <Framework/Scene2D/GrayscaleStyleConfigurator.h>
#include <Framework/Scene2D/LookupTableStyleConfigurator.h>
#include <Framework/Volumes/DicomVolumeImageMPRSlicer.h>
#include <Framework/StoneException.h>

namespace OrthancStone
{
  const char* RtViewerGuiToolToString(size_t i)
  {
    static const char* descs[] = {
      "RtViewerGuiTool_Rotate",
      "RtViewerGuiTool_Pan",
      "RtViewerGuiTool_Zoom",
      "RtViewerGuiTool_LineMeasure",
      "RtViewerGuiTool_CircleMeasure",
      "RtViewerGuiTool_AngleMeasure",
      "RtViewerGuiTool_EllipseMeasure",
      "RtViewerGuiTool_LAST"
    };
    if (i >= RtViewerGuiTool_LAST)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError, "Wrong tool index");
    }
    return descs[i];
  }

  void RtViewerApp::SelectNextTool()
  {
    currentTool_ = static_cast<RtViewerGuiTool>(currentTool_ + 1);
    if (currentTool_ == RtViewerGuiTool_LAST)
      currentTool_ = static_cast<RtViewerGuiTool>(0);;
    printf("Current tool is now: %s\n", RtViewerGuiToolToString(currentTool_));
  }

  void RtViewerApp::DisplayInfoText()
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    // do not try to use stuff too early!
    OrthancStone::ICompositor& compositor = lock->GetCompositor();

    std::stringstream msg;

    for (std::map<std::string, std::string>::const_iterator kv = infoTextMap_.begin();
         kv != infoTextMap_.end(); ++kv)
    {
      msg << kv->first << " : " << kv->second << std::endl;
    }
    std::string msgS = msg.str();

    TextSceneLayer* layerP = NULL;
    if (scene.HasLayer(FIXED_INFOTEXT_LAYER_ZINDEX))
    {
      TextSceneLayer& layer = dynamic_cast<TextSceneLayer&>(
        scene.GetLayer(FIXED_INFOTEXT_LAYER_ZINDEX));
      layerP = &layer;
    }
    else
    {
      std::unique_ptr<TextSceneLayer> layer(new TextSceneLayer);
      layerP = layer.get();
      layer->SetColor(0, 255, 0);
      layer->SetFontIndex(1);
      layer->SetBorder(20);
      layer->SetAnchor(BitmapAnchor_TopLeft);
      //layer->SetPosition(0,0);
      scene.SetLayer(FIXED_INFOTEXT_LAYER_ZINDEX, layer.release());
    }
    // position the fixed info text in the upper right corner
    layerP->SetText(msgS.c_str());
    double cX = compositor.GetCanvasWidth() * (-0.5);
    double cY = compositor.GetCanvasHeight() * (-0.5);
    scene.GetCanvasToSceneTransform().Apply(cX, cY);
    layerP->SetPosition(cX, cY);
    lock->Invalidate();
  }

  void RtViewerApp::DisplayFloatingCtrlInfoText(const PointerEvent& e)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    ScenePoint2D p = e.GetMainPosition().Apply(scene.GetCanvasToSceneTransform());

    char buf[128];
    sprintf(buf, "S:(%0.02f,%0.02f) C:(%0.02f,%0.02f)",
            p.GetX(), p.GetY(),
            e.GetMainPosition().GetX(), e.GetMainPosition().GetY());

    if (scene.HasLayer(FLOATING_INFOTEXT_LAYER_ZINDEX))
    {
      TextSceneLayer& layer =
        dynamic_cast<TextSceneLayer&>(scene.GetLayer(FLOATING_INFOTEXT_LAYER_ZINDEX));
      layer.SetText(buf);
      layer.SetPosition(p.GetX(), p.GetY());
    }
    else
    {
      std::unique_ptr<TextSceneLayer> layer(new TextSceneLayer);
      layer->SetColor(0, 255, 0);
      layer->SetText(buf);
      layer->SetBorder(20);
      layer->SetAnchor(BitmapAnchor_BottomCenter);
      layer->SetPosition(p.GetX(), p.GetY());
      scene.SetLayer(FLOATING_INFOTEXT_LAYER_ZINDEX, layer.release());
    }
  }

  void RtViewerApp::HideInfoText()
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    scene.DeleteLayer(FLOATING_INFOTEXT_LAYER_ZINDEX);
  }

  void RtViewerApp::HandleApplicationEvent(
    const SDL_Event& event)
  {
    //DisplayInfoText();

    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();
    ICompositor& compositor = lock->GetCompositor();

    if (event.type == SDL_MOUSEMOTION)
    {
      int scancodeCount = 0;
      const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

      if (activeTracker_.get() == NULL &&
          SDL_SCANCODE_LALT < scancodeCount &&
          keyboardState[SDL_SCANCODE_LALT])
      {
        // The "left-ctrl" key is down, while no tracker is present
        // Let's display the info text
        PointerEvent e;
        e.AddPosition(compositor.GetPixelCenterCoordinates(
          event.button.x, event.button.y));

        DisplayFloatingCtrlInfoText(e);
      }
      else
      {
        HideInfoText();
        //LOG(TRACE) << "(event.type == SDL_MOUSEMOTION)";
        if (activeTracker_.get() != NULL)
        {
          //LOG(TRACE) << "(activeTracker_.get() != NULL)";
          PointerEvent e;
          e.AddPosition(compositor.GetPixelCenterCoordinates(
            event.button.x, event.button.y));

          //LOG(TRACE) << "event.button.x = " << event.button.x << "     " <<
          //  "event.button.y = " << event.button.y;
          LOG(TRACE) << "activeTracker_->PointerMove(e); " <<
            e.GetMainPosition().GetX() << " " << e.GetMainPosition().GetY();

          activeTracker_->PointerMove(e);
          if (!activeTracker_->IsAlive())
            activeTracker_.reset();
        }
      }
    }
    else if (event.type == SDL_MOUSEBUTTONUP)
    {
      if (activeTracker_)
      {
        PointerEvent e;
        e.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));
        activeTracker_->PointerUp(e);
        if (!activeTracker_->IsAlive())
          activeTracker_.reset();
      }
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN)
    {
      PointerEvent e;
      e.AddPosition(compositor.GetPixelCenterCoordinates(
        event.button.x, event.button.y));
      if (activeTracker_)
      {
        activeTracker_->PointerDown(e);
        if (!activeTracker_->IsAlive())
          activeTracker_.reset();
      }
      else
      {
        // we ATTEMPT to create a tracker if need be
        activeTracker_ = CreateSuitableTracker(event, e);
      }
    }
    else if (event.type == SDL_KEYDOWN &&
             event.key.repeat == 0 /* Ignore key bounce */)
    {
      switch (event.key.keysym.sym)
      {
      case SDLK_ESCAPE:
        if (activeTracker_)
        {
          activeTracker_->Cancel();
          if (!activeTracker_->IsAlive())
            activeTracker_.reset();
        }
        break;

      case SDLK_t:
        if (!activeTracker_)
          SelectNextTool();
        else
        {
          LOG(WARNING) << "You cannot change the active tool when an interaction"
            " is taking place";
        }
        break;
      case SDLK_s:
        compositor.FitContent(scene);
        break;

      case SDLK_z:
        LOG(TRACE) << "SDLK_z has been pressed. event.key.keysym.mod == " << event.key.keysym.mod;
        if (event.key.keysym.mod & KMOD_CTRL)
        {
          if (controller.CanUndo())
          {
            LOG(TRACE) << "Undoing...";
            controller.Undo();
          }
          else
          {
            LOG(WARNING) << "Nothing to undo!!!";
          }
        }
        break;

      case SDLK_y:
        LOG(TRACE) << "SDLK_y has been pressed. event.key.keysym.mod == " << event.key.keysym.mod;
        if (event.key.keysym.mod & KMOD_CTRL)
        {
          if (controller.CanRedo())
          {
            LOG(TRACE) << "Redoing...";
            controller.Redo();
          }
          else
          {
            LOG(WARNING) << "Nothing to redo!!!";
          }
        }
        break;

      case SDLK_c:
        TakeScreenshot(
          "screenshot.png",
          compositor.GetCanvasWidth(),
          compositor.GetCanvasHeight());
        break;

      default:
        break;
      }
    }
    else if (viewport_->IsRefreshEvent(event))
    {
      viewport_->Paint();
    }
  }

  void RtViewerApp::OnSceneTransformChanged(
    const ViewportController::SceneTransformChanged& message)
  {
    DisplayInfoText();
  }

  void RtViewerApp::RetrieveGeometry()
  {
    ORTHANC_ASSERT(geometryProvider_.get() != NULL);
    ORTHANC_ASSERT(geometryProvider_->HasGeometry());
    const VolumeImageGeometry& geometry = geometryProvider_->GetImageGeometry();

    const unsigned int depth = geometry.GetProjectionDepth(projection_);
    currentPlane_ = depth / 2;

    planes_.resize(depth);

    for (unsigned int z = 0; z < depth; z++)
    {
      planes_[z] = geometry.GetProjectionSlice(projection_, z);
    }

    UpdateLayers();

    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
    lock->GetCompositor().FitContent(lock->GetController().GetScene());
    lock->Invalidate();
  }

  void RtViewerApp::FitContent()
  {
    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
    lock->GetCompositor().FitContent(lock->GetController().GetScene());
    lock->Invalidate();
  }

  void RtViewerApp::UpdateLayers()
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    if ((planes_.size() == 0)
        && (geometryProvider_.get() != NULL)
        && (geometryProvider_->HasGeometry()))
    {
      RetrieveGeometry();
    }

    if (currentPlane_ < planes_.size())
    {
      if (ctVolumeLayerSource_.get() != NULL)
      {
        ctVolumeLayerSource_->Update(planes_[currentPlane_]);
      }
      if (doseVolumeLayerSource_.get() != NULL)
      {
        doseVolumeLayerSource_->Update(planes_[currentPlane_]);
      }
      if (structLayerSource_.get() != NULL)
      {
        structLayerSource_->Update(planes_[currentPlane_]);
      }
    }
    lock->Invalidate();
  }

  boost::shared_ptr<IFlexiblePointerTracker> RtViewerApp::CreateSuitableTracker(
    const SDL_Event& event,
    const PointerEvent& e)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();
    ICompositor& compositor = lock->GetCompositor();

    using namespace Orthanc;

    switch (event.button.button)
    {
    case SDL_BUTTON_MIDDLE:
      return boost::shared_ptr<IFlexiblePointerTracker>(new PanSceneTracker
      (viewport_, e));

    case SDL_BUTTON_RIGHT:
      return boost::shared_ptr<IFlexiblePointerTracker>(new ZoomSceneTracker
      (viewport_, e, compositor.GetCanvasHeight()));

    case SDL_BUTTON_LEFT:
    {
      //LOG(TRACE) << "CreateSuitableTracker: case SDL_BUTTON_LEFT:";
      // TODO: we need to iterate on the set of measuring tool and perform
      // a hit test to check if a tracker needs to be created for edition.
      // Otherwise, depending upon the active tool, we might want to create
      // a "measuring tool creation" tracker

      // TODO: if there are conflicts, we should prefer a tracker that 
      // pertains to the type of measuring tool currently selected (TBD?)
      boost::shared_ptr<IFlexiblePointerTracker> hitTestTracker = TrackerHitTest(e);

      if (hitTestTracker != NULL)
      {
        //LOG(TRACE) << "hitTestTracker != NULL";
        return hitTestTracker;
      }
      else
      {
        switch (currentTool_)
        {
        case RtViewerGuiTool_Rotate:
          //LOG(TRACE) << "Creating RotateSceneTracker";
          return boost::shared_ptr<IFlexiblePointerTracker>(new RotateSceneTracker(viewport_, e));
        case RtViewerGuiTool_Pan:
          return boost::shared_ptr<IFlexiblePointerTracker>(new PanSceneTracker(viewport_, e));
        case RtViewerGuiTool_Zoom:
          return boost::shared_ptr<IFlexiblePointerTracker>(new ZoomSceneTracker(viewport_, e, compositor.GetCanvasHeight()));
        //case GuiTool_AngleMeasure:
        //  return new AngleMeasureTracker(GetScene(), e);
        //case GuiTool_CircleMeasure:
        //  return new CircleMeasureTracker(GetScene(), e);
        //case GuiTool_EllipseMeasure:
        //  return new EllipseMeasureTracker(GetScene(), e);
        case RtViewerGuiTool_LineMeasure:
          return boost::shared_ptr<IFlexiblePointerTracker>(new CreateLineMeasureTracker(viewport_, e));
        case RtViewerGuiTool_AngleMeasure:
          return boost::shared_ptr<IFlexiblePointerTracker>(new CreateAngleMeasureTracker(viewport_, e));
        case RtViewerGuiTool_CircleMeasure:
          LOG(ERROR) << "Not implemented yet!";
          return boost::shared_ptr<IFlexiblePointerTracker>();
        case RtViewerGuiTool_EllipseMeasure:
          LOG(ERROR) << "Not implemented yet!";
          return boost::shared_ptr<IFlexiblePointerTracker>();
        default:
          throw OrthancException(ErrorCode_InternalError, "Wrong tool!");
        }
      }
    }
    default:
      return boost::shared_ptr<IFlexiblePointerTracker>();
    }
  }


  RtViewerApp::RtViewerApp()
    : oracle_(*this)
    , currentTool_(RtViewerGuiTool_Rotate)
    , undoStack_(new UndoStack)
    , currentPlane_(0)
    , projection_(VolumeProjection_Coronal)
  {
    loadersContext_.reset(new GenericLoadersContext(1, 4, 1));
    loadersContext_->StartOracle();

    // False means we do NOT let Windows treat this as a legacy application that needs to be scaled
    viewport_ = SdlOpenGLViewport::Create("CT RTDOSE RTSTRUCT viewer", 1024, 1024, false);

    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    //oracleObservable.RegisterObserverCallback
    //(new Callable
    //  <RtViewerApp, SleepOracleCommand::TimeoutMessage>(*this, &RtViewerApp::Handle));

    //oracleObservable.RegisterObserverCallback
    //(new Callable
    //  <Toto, GetOrthancImageCommand::SuccessMessage>(*this, &RtViewerApp::Handle));

    //oracleObservable.RegisterObserverCallback
    //(new Callable
    //  <RtViewerApp, GetOrthancWebViewerJpegCommand::SuccessMessage>(*this, &ToRtViewerAppto::Handle));


    TEXTURE_2x2_1_ZINDEX = 1;
    TEXTURE_1x1_ZINDEX = 2;
    TEXTURE_2x2_2_ZINDEX = 3;
    LINESET_1_ZINDEX = 4;
    LINESET_2_ZINDEX = 5;
    FLOATING_INFOTEXT_LAYER_ZINDEX = 6;
    FIXED_INFOTEXT_LAYER_ZINDEX = 7;
  }

  void RtViewerApp::RegisterMessages()
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();
    Register<OracleCommandExceptionMessage>(oracleObservable_, &RtViewerApp::Handle);
    Register<ViewportController::SceneTransformChanged>(controller, &RtViewerApp::OnSceneTransformChanged);
  }

  boost::shared_ptr<RtViewerApp> RtViewerApp::Create()
  {
    boost::shared_ptr<RtViewerApp> thisOne(new RtViewerApp());
    thisOne->RegisterMessages();
    return thisOne;
  }

  void RtViewerApp::PrepareScene()
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    // Texture of 2x2 size
    {
      Orthanc::Image i(Orthanc::PixelFormat_RGB24, 2, 2, false);

      uint8_t* p = reinterpret_cast<uint8_t*>(i.GetRow(0));
      p[0] = 255;
      p[1] = 0;
      p[2] = 0;

      p[3] = 0;
      p[4] = 255;
      p[5] = 0;

      p = reinterpret_cast<uint8_t*>(i.GetRow(1));
      p[0] = 0;
      p[1] = 0;
      p[2] = 255;

      p[3] = 255;
      p[4] = 0;
      p[5] = 0;

      scene.SetLayer(TEXTURE_2x2_1_ZINDEX, new ColorTextureSceneLayer(i));
    }
  }

  void RtViewerApp::DisableTracker()
  {
    if (activeTracker_)
    {
      activeTracker_->Cancel();
      activeTracker_.reset();
    }
  }

  void RtViewerApp::TakeScreenshot(const std::string& target,
                                                unsigned int canvasWidth,
                                                unsigned int canvasHeight)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    CairoCompositor compositor(canvasWidth, canvasHeight);
    compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, FONT_SIZE_0, Orthanc::Encoding_Latin1);
    compositor.Refresh(scene);

    Orthanc::ImageAccessor canvas;
    compositor.GetCanvas().GetReadOnlyAccessor(canvas);

    Orthanc::Image png(Orthanc::PixelFormat_RGB24, canvas.GetWidth(), canvas.GetHeight(), false);
    Orthanc::ImageProcessing::Convert(png, canvas);

    Orthanc::PngWriter writer;
    writer.WriteToFile(target, png);
  }

  boost::shared_ptr<IFlexiblePointerTracker> RtViewerApp::TrackerHitTest(const PointerEvent& e)
  {
    // std::vector<boost::shared_ptr<MeasureTool>> measureTools_;
    return boost::shared_ptr<IFlexiblePointerTracker>();
  }

  static void GLAPIENTRY
    OpenGLMessageCallback(GLenum source,
                          GLenum type,
                          GLuint id,
                          GLenum severity,
                          GLsizei length,
                          const GLchar* message,
                          const void* userParam)
  {
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
    {
      fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
              (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
              type, severity, message);
    }
  }

  static bool g_stopApplication = false;


  void RtViewerApp::Handle(const DicomVolumeImage::GeometryReadyMessage& message)
  {
    RetrieveGeometry();
  }


  void RtViewerApp::Handle(const OracleCommandExceptionMessage& message)
  {
    const OracleCommandBase& command = dynamic_cast<const OracleCommandBase&>(message.GetOrigin());

    printf("EXCEPTION: [%s] on command type %d\n", message.GetException().What(), command.GetType());

    switch (command.GetType())
    {
    case IOracleCommand::Type_GetOrthancWebViewerJpeg:
      printf("URI: [%s]\n", dynamic_cast<const GetOrthancWebViewerJpegCommand&>(command).GetUri().c_str());
      break;

    default:
      break;
    }
  }


  void RtViewerApp::HandleCTLoaded(const OrthancSeriesVolumeProgressiveLoader::VolumeImageReadyInHighQuality& message)
  {
    UpdateLayers();
  }

  void RtViewerApp::HandleCTContentUpdated(const DicomVolumeImage::ContentUpdatedMessage& message)
  {
    UpdateLayers();
  }

  void RtViewerApp::HandleDoseLoaded(const DicomVolumeImage::ContentUpdatedMessage& message)
  {
    //TODO: compute dose extent, with outlier rejection
    UpdateLayers();
  }

  void RtViewerApp::HandleStructuresReady(const DicomStructureSetLoader::StructuresReady& message)
  {
    UpdateLayers();
  }

  void RtViewerApp::HandleStructuresUpdated(const DicomStructureSetLoader::StructuresUpdated& message)
  {
    UpdateLayers();
  }

  void RtViewerApp::SetCtVolume(int depth,
                                            const boost::shared_ptr<OrthancStone::IVolumeSlicer>& volume,
                                            OrthancStone::ILayerStyleConfigurator* style)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    ctVolumeLayerSource_.reset(new OrthancStone::VolumeSceneLayerSource(scene, depth, volume));

    if (style != NULL)
    {
      ctVolumeLayerSource_->SetConfigurator(style);
    }
  }

  void RtViewerApp::SetDoseVolume(int depth,
                                            const boost::shared_ptr<OrthancStone::IVolumeSlicer>& volume,
                                            OrthancStone::ILayerStyleConfigurator* style)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    doseVolumeLayerSource_.reset(new OrthancStone::VolumeSceneLayerSource(scene, depth, volume));

    if (style != NULL)
    {
      doseVolumeLayerSource_->SetConfigurator(style);
    }
  }

  void RtViewerApp::SetStructureSet(int depth,
                                                 const boost::shared_ptr<OrthancStone::DicomStructureSetLoader>& volume)
  {
    std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
    ViewportController& controller = lock->GetController();
    Scene2D& scene = controller.GetScene();

    structLayerSource_.reset(new OrthancStone::VolumeSceneLayerSource(scene, depth, volume));
  }

  void RtViewerApp::Run()
  {
    {
      std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
      ViewportController& controller = lock->GetController();
      Scene2D& scene = controller.GetScene();
      ICompositor& compositor = lock->GetCompositor();

      // False means we do NOT let Windows treat this as a legacy application
      // that needs to be scaled
      controller.FitContent(compositor.GetCanvasWidth(), compositor.GetCanvasHeight());

      glEnable(GL_DEBUG_OUTPUT);
      glDebugMessageCallback(OpenGLMessageCallback, 0);

      compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_0, Orthanc::Encoding_Latin1);
      compositor.SetFont(1, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_1, Orthanc::Encoding_Latin1);
    }
                     //////// from loader
    {
      Orthanc::WebServiceParameters p;
      //p.SetUrl("http://localhost:8043/");
      p.SetCredentials("orthanc", "orthanc");
      oracle_.SetOrthancParameters(p);
    }

    //////// from Run

    boost::shared_ptr<DicomVolumeImage>  ctVolume(new DicomVolumeImage);
    boost::shared_ptr<DicomVolumeImage>  doseVolume(new DicomVolumeImage);


    boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader> ctLoader;
    boost::shared_ptr<OrthancMultiframeVolumeLoader> doseLoader;
    boost::shared_ptr<DicomStructureSetLoader>  rtstructLoader;

    {
      // "true" means use progressive quality (jpeg 50 --> jpeg 90 --> 16-bit raw)
      ctLoader = OrthancSeriesVolumeProgressiveLoader::Create(*loadersContext_, ctVolume, false);
      
      // we need to store the CT loader to ask from geometry details later on when geometry is loaded
      geometryProvider_ = ctLoader;

      doseLoader = OrthancMultiframeVolumeLoader::Create(*loadersContext_, doseVolume);
      rtstructLoader = DicomStructureSetLoader::Create(*loadersContext_);
    }

    Register<DicomVolumeImage::GeometryReadyMessage>(*ctLoader, &RtViewerApp::Handle);
    Register<OrthancSeriesVolumeProgressiveLoader::VolumeImageReadyInHighQuality>(*ctLoader, &RtViewerApp::HandleCTLoaded);
    Register<DicomVolumeImage::ContentUpdatedMessage>(*ctLoader, &RtViewerApp::HandleCTContentUpdated);
    Register<DicomVolumeImage::ContentUpdatedMessage>(*doseLoader, &RtViewerApp::HandleDoseLoaded);
    Register<DicomStructureSetLoader::StructuresReady>(*rtstructLoader, &RtViewerApp::HandleStructuresReady);
    Register<DicomStructureSetLoader::StructuresUpdated>(*rtstructLoader, &RtViewerApp::HandleStructuresUpdated);

    std::auto_ptr<GrayscaleStyleConfigurator> style(new GrayscaleStyleConfigurator);
    style->SetLinearInterpolation(true);

    this->SetCtVolume(LAYER_POSITION + 0, ctLoader, style.release());

    {
      std::unique_ptr<LookupTableStyleConfigurator> config(new LookupTableStyleConfigurator);
      config->SetLookupTable(Orthanc::EmbeddedResources::COLORMAP_HOT);

      boost::shared_ptr<DicomVolumeImageMPRSlicer> tmp(new DicomVolumeImageMPRSlicer(doseVolume));
      this->SetDoseVolume(LAYER_POSITION + 1, tmp, config.release());
    }

    this->SetStructureSet(LAYER_POSITION + 2, rtstructLoader);

#if 1
    /*
    BGO data
    http://localhost:8042/twiga-orthanc-viewer-demo/twiga-orthanc-viewer-demo.html?ct-series=a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa
    &
    dose-instance=830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb
    &
    struct-instance=54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9
    */
    ctLoader->LoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");  // CT
    doseLoader->LoadInstance("830a69ff-8e4b5ee3-b7f966c8-bccc20fb-d322dceb");  // RT-DOSE
    rtstructLoader->LoadInstance("54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9");  // RT-STRUCT
#else
    //ctLoader->LoadSeries("cb3ea4d1-d08f3856-ad7b6314-74d88d77-60b05618");  // CT
    //doseLoader->LoadInstance("41029085-71718346-811efac4-420e2c15-d39f99b6");  // RT-DOSE
    //rtstructLoader->LoadInstance("83d9c0c3-913a7fee-610097d7-cbf0522d-fd75bee6");  // RT-STRUCT

    // 2017-05-16
    ctLoader->LoadSeries("a04ecf01-79b2fc33-58239f7e-ad9db983-28e81afa");  // CT
    doseLoader->LoadInstance("eac822ef-a395f94e-e8121fe0-8411fef8-1f7bffad");  // RT-DOSE
    rtstructLoader->LoadInstance("54460695-ba3885ee-ddf61ac0-f028e31d-a6e474d9");  // RT-STRUCT
#endif

    oracle_.Start();

//// END from loader

    while (!g_stopApplication)
    {
      //compositor.Refresh(scene);

//////// from loader
//// END from loader

      SDL_Event event;
      while (!g_stopApplication && SDL_PollEvent(&event))
      {
        if (event.type == SDL_QUIT)
        {
          g_stopApplication = true;
          break;
        }
        else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
          DisableTracker(); // was: tracker.reset(NULL);
        }
        else if (event.type == SDL_KEYDOWN &&
                 event.key.repeat == 0 /* Ignore key bounce */)
        {
          switch (event.key.keysym.sym)
          {
          case SDLK_f:
            // TODO: implement GetWindow!!!
            // viewport_->GetContext()->GetWindow().ToggleMaximize();
            ORTHANC_ASSERT(false, "Please implement GetWindow()");
            break;

          case SDLK_r:
            break;

          case SDLK_s:
            FitContent();
            break;

          case SDLK_q:
            g_stopApplication = true;
            break;
          default:
            break;
          }
        }
        HandleApplicationEvent(event);
      }
      SDL_Delay(1);
    }

    //// from loader

    //Orthanc::SystemToolbox::ServerBarrier();

    /**
     * WARNING => The oracle must be stopped BEFORE the objects using
     * it are destroyed!!! This forces to wait for the completion of
     * the running callback methods. Otherwise, the callbacks methods
     * might still be running while their parent object is destroyed,
     * resulting in crashes. This is very visible if adding a sleep(),
     * as in (*).
     **/

    oracle_.Stop();
    //// END from loader
  }

  void RtViewerApp::SetInfoDisplayMessage(
    std::string key, std::string value)
  {
    if (value == "")
      infoTextMap_.erase(key);
    else
      infoTextMap_[key] = value;
    DisplayInfoText();
  }

}


boost::weak_ptr<OrthancStone::RtViewerApp> g_app;

void RtViewer_SetInfoDisplayMessage(std::string key, std::string value)
{
  boost::shared_ptr<OrthancStone::RtViewerApp> app = g_app.lock();
  if (app)
  {
    app->SetInfoDisplayMessage(key, value);
  }
}

/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  using namespace OrthancStone;

  StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);
  //Orthanc::Logging::EnableTraceLevel(true);

  try
  {
    boost::shared_ptr<RtViewerApp> app = RtViewerApp::Create();
    g_app = app;
    //app->PrepareScene();
    app->Run();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  StoneFinalize();

  return 0;
}


