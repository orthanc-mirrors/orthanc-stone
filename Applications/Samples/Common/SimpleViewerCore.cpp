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

#include "../../../OrthancStone/Sources/Scene2DViewport/UndoStack.h"

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
      units_ = Units_Millimeters;
    }
    else
    {
      LOG(INFO) << "Using pixels units, as the DICOM instance does *not* contain the PixelSpacing tag";
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


  void SimpleViewerCore::CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                          const boost::shared_ptr<IViewport>& viewport)
  {
    context_ = context;
    viewport_ = viewport;

    dicomLoader_ = DicomResourcesLoader::Create(*context);

    //Register<DicomResourcesLoadedMessage>(*dicomLoader_, &SimpleViewerCore::Handle);

    //dicomLoader_->TypedObservable<DicomResourcesLoadedMessage>::Register(shared_from_this());
    New::IObservable::Bind<DicomResourcesLoadedMessage>(dicomLoader_, shared_from_this());

    annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_Edit);
    annotations_.SetProbedLayer(0);

    interactor_.SetWindowingLayer(0);

    {
      std::unique_ptr<IViewport::ILock> viewportLock(viewport->Lock());
      viewportLock->GetController().SetUndoStack(boost::make_shared<UndoStack>());
    }
  }


  bool SimpleViewerCore::HandleKeyDown(const IEnvironment::ILock& environmentLock,
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
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_Circle);
      break;

    case 'm':
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_Edit);
      break;

    case 'd':
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_Remove);
      break;

    case 'l':
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_Length);
      break;

    case 'a':
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_Angle);
      break;

    case 'p':
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_PixelProbe);
      break;

    case 'e':
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_EllipseProbe);
      break;

    case 'r':
      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_RectangleProbe);
      break;

    default:
      break;
    }

    return false;  // No need to repaint
  }


  void SimpleViewerCore::HandleMouseDown(const IEnvironment::ILock& environmentLock,
                                         IViewport::ILock& viewportLock,
                                         const PointerEvent& p)
  {
    annotations_.SetUnits(units_);

    boost::shared_ptr<IFlexiblePointerTracker> t;

    if (p.GetMouseButton() == MouseButton_Left)
    {
      t.reset(annotations_.CreateTracker(p.GetMainPosition(), viewportLock.GetController().GetScene()));
    }

    if (t.get() != NULL)
    {
      viewportLock.GetController().AcquireActiveTracker(t);
    }
    else
    {
      viewportLock.GetController().HandleMousePress(interactor_, p,
                                                    viewportLock.GetCompositor().GetCanvasWidth(),
                                                    viewportLock.GetCompositor().GetCanvasHeight());
    }

    viewportLock.Invalidate();
  }


  bool SimpleViewerCore::HandleMouseMove(const IEnvironment::ILock& environmentLock,
                                         IViewport::ILock& viewportLock,
                                         const PointerEvent& p)
  {
    if (viewportLock.GetController().HandleMouseMove(p))
    {
      viewportLock.Invalidate();
      if (annotations_.ClearHover())
      {
        return true;
      }
    }
    else
    {
      if (annotations_.SetMouseHover(p.GetMainPosition(), viewportLock.GetController().GetScene()))
      {
        return true;
      }
    }

    return false;
  }


  void SimpleViewerCore::Render(const IEnvironment::ILock& environmentLock,
                                IViewport::ILock& viewportLock)
  {
    annotations_.Render(viewportLock.GetController().GetScene());
  }


  SimpleViewerCore::SimpleViewerCore() :
    annotations_(10),
    units_(Units_Pixels)
  {
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
