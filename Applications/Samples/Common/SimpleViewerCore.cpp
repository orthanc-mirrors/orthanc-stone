/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include "SimpleViewerCore.h"

// TODO Refactoring - Remove
#define TEST_LINE_MEASURE_TOOL   1
#define TEST_ANGLE_MEASURE_TOOL  1


#include "../../../OrthancStone/Sources/Scene2DViewport/UndoStack.h"
#include "../../../OrthancStone/Sources/Viewport/DefaultViewportInteractor.h"


#include "../../../OrthancStone/Sources/Scene2DViewport/LineMeasureTool.h"  // TODO Refactoring - Remove
#include "../../../OrthancStone/Sources/Scene2DViewport/AngleMeasureTool.h"  // TODO Refactoring - Remove
#include "../../../OrthancStone/Sources/Scene2DViewport/MeasureCommands.h"  // TODO Refactoring - Remove


#include <boost/make_shared.hpp>


namespace OrthancStone
{
  void SimpleViewerCore::Handle(const New::IObservable& observable,
                                const FrameDecodedMessage& message)
  {
    LOG(INFO) << "Frame decoded! "
              << message.GetImage().GetWidth() << "x" << message.GetImage().GetHeight()
              << " " << Orthanc::EnumerationToString(message.GetImage().GetFormat());

    std::unique_ptr<TextureBaseSceneLayer> layer(
      message.GetInstanceParameters().CreateTexture(message.GetImage()));
    //layer->SetLinearInterpolation(true);
    layer->SetLinearInterpolation(false);

    {
      std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());
      viewportLock->GetController().GetScene().SetLayer(0, layer.release());
      viewportLock->GetCompositor().FitContent(viewportLock->GetController().GetScene());
      viewportLock->Invalidate();
    }
  }


  void SimpleViewerCore::Handle(const New::IObservable& observable,
                                const DicomResourcesLoadedMessage& message)
  {
    if (message.GetResources()->GetSize() != 1)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    DicomInstanceParameters parameters(message.GetResources()->GetResource(0));
    if (parameters.HasPixelSpacing())
    {
      /**
       * TODO - Ultrasound (US) images store an equivalent to
       * "PixelSpacing" in the "SequenceOfUltrasoundRegions"
       * (0018,6011) sequence, cf. tags "PhysicalDeltaX" (0018,602c)
       * and "PhysicalDeltaY" (0018,602e) => This would require
       * parsing "message.GetResources()->GetSourceJson(0)"
       * => cf. "DicomInstanceParameters::EnrichUsingDicomWeb()"
       **/

      // std::cout << message.GetResources()->GetSourceJson(0).toStyledString();

      LOG(INFO) << "Using millimeters units, as the DICOM instance contains the PixelSpacing tag";
      annotations_->SetUnits(Units_Millimeters);
    }
    else
    {
      LOG(INFO) << "Using pixels units, as the DICOM instance does *not* contain the PixelSpacing tag";
      annotations_->SetUnits(Units_Pixels);
    }

    //message.GetResources()->GetResource(0).Print(stdout);

    {
      std::unique_ptr<IEnvironment::ILock> environmentLock(context_->GetEnvironment().AcquireLock());  // TODO Refactoring - Should already be locked

      framesLoader_ = SeriesFramesLoader::Create(*context_, *message.GetResources());

      //Register<FrameDecodedMessage>(*framesLoader_, &SimpleViewerCore::Handle);

      //framesLoader_->TypedObservable<FrameDecodedMessage>::Register(shared_from_this());
      New::IObservable::Bind<FrameDecodedMessage>(framesLoader_, shared_from_this());

      assert(message.HasUserPayload());

      const Orthanc::SingleValueObject<unsigned int>& payload =
        dynamic_cast<const Orthanc::SingleValueObject<unsigned int>&>(
          message.GetUserPayload());

      LOG(INFO) << "Loading pixel data of frame: " << payload.GetValue();
      framesLoader_->ScheduleLoadFrame(
        0, message.GetDicomSource(), payload.GetValue(),
        message.GetDicomSource().GetQualityCount() - 1 /* download best quality available */,
        NULL);
    }
  }


  void SimpleViewerCore::RenderAnnotations()
  {
    // This is necessary to update the size of handles in the annotations layer while zoom level changes
    std::unique_ptr<OrthancStone::IViewport::ILock> lock(viewport_->Lock());
    annotations_->Render(lock->GetController().GetScene());
    lock->Invalidate();
  }


  void SimpleViewerCore::CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                          const boost::shared_ptr<IViewport>& viewport)
  {
    if (context_ ||
        viewport_ ||
        annotations_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    context_ = context;
    viewport_ = viewport;

    dicomLoader_ = DicomResourcesLoader::Create(*context);

    //Register<DicomResourcesLoadedMessage>(*dicomLoader_, &SimpleViewerCore::Handle);

    //dicomLoader_->TypedObservable<DicomResourcesLoadedMessage>::Register(shared_from_this());
    New::IObservable::Bind<DicomResourcesLoadedMessage>(dicomLoader_, shared_from_this());

    annotations_.reset(new AnnotationsSceneLayer(10));
    annotations_->SetUnits(Units_Millimeters);
    annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_Edit);
    annotations_->SetProbedLayer(0);

    Register<AnnotationsSceneLayer::AnnotationRemovedMessage>(*annotations_, &SimpleViewerCore::Handle);

    {
      std::unique_ptr<IViewport::ILock> viewportLock(viewport->Lock());
      viewportLock->GetController().SetUndoStack(boost::make_shared<UndoStack>());
    }

#if TEST_LINE_MEASURE_TOOL == 1
    {
      boost::shared_ptr<LineMeasureTool> tool = OrthancStone::LineMeasureTool::Create(viewport);
      tool->Enable();
      tool->Set(OrthancStone::ScenePoint2D(200, 100),
                OrthancStone::ScenePoint2D(100, 200));

      std::unique_ptr<IViewport::ILock> viewportLock(viewport->Lock());
      viewportLock->GetController().AddMeasureTool(boost::dynamic_pointer_cast<MeasureTool>(tool));
    }
#endif

#if TEST_ANGLE_MEASURE_TOOL == 1
    {
      boost::shared_ptr<AngleMeasureTool> tool = OrthancStone::AngleMeasureTool::Create(viewport);
      tool->Enable();
      tool->SetCenter(OrthancStone::ScenePoint2D(300, 300));
      tool->SetSide1End(OrthancStone::ScenePoint2D(200, 400));
      tool->SetSide2End(OrthancStone::ScenePoint2D(400, 400));

      std::unique_ptr<IViewport::ILock> viewportLock(viewport->Lock());
      viewportLock->GetController().AddMeasureTool(boost::dynamic_pointer_cast<MeasureTool>(tool));
    }
#endif
  }


  class SimpleViewerCore::Interactor : public IViewportInteractor
  {
  private:
    class Observer : public IFlexiblePointerTracker::IObserver
    {
    private:
      boost::shared_ptr<SimpleViewerCore>  core_;

    public:
      Observer(const boost::shared_ptr<SimpleViewerCore>& core) :
        core_(core)
      {
        assert(core_);
      }

      virtual void HandleGrayscaleWindowingChange(const OrthancStone::Windowing& windowing) ORTHANC_OVERRIDE
      {
      }

      virtual void HandleSceneTransformChange(const OrthancStone::Scene2D& scene) ORTHANC_OVERRIDE
      {
        core_->RenderAnnotations();
      }
    };

    class Factory : public DefaultViewportInteractor::IObserverFactory
    {
    private:
      boost::shared_ptr<SimpleViewerCore>  core_;

    public:
      Factory(const boost::shared_ptr<SimpleViewerCore>& core) :
        core_(core)
      {
        assert(core_);
      }

      virtual IFlexiblePointerTracker::IObserver* CreateObserver()
      {
        return new Observer(core_);
      }
    };

    DefaultViewportInteractor            default_;
    boost::shared_ptr<SimpleViewerCore>  core_;

  public:
    Interactor(const boost::shared_ptr<SimpleViewerCore>& core) :
      core_(core)
    {
      if (!core)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      default_.SetWindowingLayer(0);
      default_.SetObserverFactory(new Factory(core));
    }

    virtual IFlexiblePointerTracker* CreateTracker(Scene2D& scene,
                                                   const PointerEvent& event,
                                                   unsigned int viewportWidth,
                                                   unsigned int viewportHeight) ORTHANC_OVERRIDE
    {
#if TEST_LINE_MEASURE_TOOL == 1
      const ScenePoint2D p = event.GetMainPosition().Apply(scene.GetCanvasToSceneTransform());

      std::vector< boost::shared_ptr<MeasureTool> > tools;

      {
        std::unique_ptr<IViewport::ILock> viewportLock(core_->viewport_->Lock());
        tools = viewportLock->GetController().HitTestMeasureTools(p);
      }

      if (!tools.empty())
      {
        return tools[0]->CreateEditionTracker(false, event);
      }
      else
      {
        switch (core_->annotations_->GetActiveTool())
        {
          case AnnotationsSceneLayer::Tool_Remove:
            LOG(WARNING) << "Nothing to delete there";
            break;

          case AnnotationsSceneLayer::Tool_Length:
          {
            boost::shared_ptr<LineMeasureTool> tool = OrthancStone::LineMeasureTool::Create(core_->viewport_);
            tool->Enable();
            tool->Set(p, p);

            {
              std::unique_ptr<IViewport::ILock> viewportLock(core_->viewport_->Lock());
              viewportLock->GetController().AddMeasureTool(boost::dynamic_pointer_cast<MeasureTool>(tool));
            }

            return tool->CreateEditionTracker(true, event);
          }

          case AnnotationsSceneLayer::Tool_Angle:

          default:
            LOG(ERROR) << "This tool is not supported if not using the annotations layer";
        }

        return NULL;
      }
#endif

      core_->annotations_->ClearHover();

      if (event.GetMouseButton() == MouseButton_Left)
      {
        return core_->annotations_->CreateTracker(event.GetMainPosition(), scene);
      }
      else
      {
        return default_.CreateTracker(scene, event, viewportWidth, viewportHeight);
      }
    }

    virtual bool HasMouseHover() const ORTHANC_OVERRIDE
    {
      return true;
    }

    virtual void HandleMouseHover(Scene2D& scene,
                                  const PointerEvent& event) ORTHANC_OVERRIDE
    {
      core_->annotations_->SetMouseHover(event.GetMainPosition(), scene);
    }
  };


  IViewportInteractor* SimpleViewerCore::CreateMouseInteractor()
  {
    if (annotations_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return new Interactor(shared_from_this());
    }
  }


  void SimpleViewerCore::HandleKeyDown(const IEnvironment::ILock& environmentLock,
                                       char key)
  {
    switch (key)
    {
      case 's':
      {
        std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());
        viewportLock->GetCompositor().FitContent(viewportLock->GetController().GetScene());
        viewportLock->Invalidate();
        break;
      }

      case 'u':
      {
        std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());
        if (viewportLock->GetController().CanUndo())
        {
          viewportLock->GetController().Undo();
        }
        break;
      }

      case 'U':
      {
        std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());
        if (viewportLock->GetController().CanRedo())
        {
          viewportLock->GetController().Redo();
        }
        break;
      }

      case 'c':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_Circle);
        break;

      case 'm':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_Edit);
        break;

      case 'd':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_Remove);
        break;

      case 'l':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_Length);
        break;

      case 'a':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_Angle);
        break;

      case 'p':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_PixelProbe);
        break;

      case 'e':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_EllipseProbe);
        break;

      case 'r':
        annotations_->SetActiveTool(AnnotationsSceneLayer::Tool_RectangleProbe);
        break;

      default:
        break;
    }
  }


  void SimpleViewerCore::LoadFrameFromOrthanc(const std::string& instanceId,
                                              unsigned int frameIndex)
  {
    std::unique_ptr<IEnvironment::ILock> environmentLock(context_->GetEnvironment().AcquireLock());

    DicomSource source;
    dicomLoader_->ScheduleLoadOrthancResource(boost::make_shared<LoadedDicomResources>(Orthanc::DICOM_TAG_SOP_INSTANCE_UID),
                                              0, source, Orthanc::ResourceType_Instance, instanceId,
                                              new Orthanc::SingleValueObject<unsigned int>(frameIndex));
  }


  void SimpleViewerCore::LoadFrameFromDicomWeb(const std::string& server,
                                               const std::string& studyInstanceUid,
                                               const std::string& seriesInstanceUid,
                                               const std::string& sopInstanceUid,
                                               unsigned int frameIndex)
  {
    OrthancStone::DicomSource source;
    source.SetDicomWebThroughOrthancSource(server);

    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }
}
