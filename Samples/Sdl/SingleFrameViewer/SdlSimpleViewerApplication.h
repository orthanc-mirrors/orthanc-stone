#pragma once

#include <Framework/Viewport/IViewport.h>
#include <Framework/Loaders/DicomResourcesLoader.h>
#include <Framework/Loaders/ILoadersContext.h>
#include <Framework/Loaders/SeriesFramesLoader.h>
#include <Framework/Loaders/SeriesThumbnailsLoader.h>

#include <boost/make_shared.hpp>


using OrthancStone::ILoadersContext;
using OrthancStone::ObserverBase;
using OrthancStone::IViewport;
using OrthancStone::DicomResourcesLoader;
using OrthancStone::SeriesFramesLoader;
using OrthancStone::TextureBaseSceneLayer;
using OrthancStone::DicomSource;
using OrthancStone::SeriesThumbnailsLoader;
using OrthancStone::LoadedDicomResources;
using OrthancStone::SeriesThumbnailType;
using OrthancStone::OracleScheduler;
using OrthancStone::OrthancRestApiCommand;
using OrthancStone::OracleScheduler;
using OrthancStone::OracleScheduler;
using OrthancStone::OracleScheduler;


class SdlSimpleViewerApplication : public ObserverBase<SdlSimpleViewerApplication>
{

public:
  static boost::shared_ptr<SdlSimpleViewerApplication> Create(ILoadersContext& context, boost::shared_ptr<IViewport> viewport)
  {
    boost::shared_ptr<SdlSimpleViewerApplication> application(new SdlSimpleViewerApplication(context, viewport));

    {
      std::auto_ptr<ILoadersContext::ILock> lock(context.Lock());
      DicomResourcesLoader::Factory f;
      application->dicomLoader_ = boost::dynamic_pointer_cast<DicomResourcesLoader>(f.Create(*lock));
    }

    application->Register<DicomResourcesLoader::SuccessMessage>(*application->dicomLoader_, &SdlSimpleViewerApplication::Handle);

    return application;
  }

  void LoadOrthancFrame(const DicomSource& source, const std::string& instanceId, unsigned int frame)
  {
    std::auto_ptr<ILoadersContext::ILock> lock(context_.Lock());

    dicomLoader_->ScheduleLoadOrthancResource(boost::make_shared<LoadedDicomResources>(Orthanc::DICOM_TAG_SOP_INSTANCE_UID),
                                              0, source, Orthanc::ResourceType_Instance, instanceId,
                                              new Orthanc::SingleValueObject<unsigned int>(frame));
  }

#if 0
  void LoadDicomWebFrame(const DicomSource& source,
                         const std::string& studyInstanceUid,
                         const std::string& seriesInstanceUid,
                         const std::string& sopInstanceUid,
                         unsigned int frame)
  {
    std::auto_ptr<ILoadersContext::ILock> lock(context_.Lock());

    // We first must load the "/metadata" to know the number of frames
    dicomLoader_->ScheduleGetDicomWeb(
      boost::make_shared<LoadedDicomResources>(Orthanc::DICOM_TAG_SOP_INSTANCE_UID), 0, source,
      "/studies/" + studyInstanceUid + "/series/" + seriesInstanceUid + "/instances/" + sopInstanceUid + "/metadata",
      new Orthanc::SingleValueObject<unsigned int>(frame));
  }
#endif 

  void FitContent()
  {
    std::auto_ptr<IViewport::ILock> lock(viewport_->Lock());
    lock->GetCompositor().FitContent(lock->GetController().GetScene());
    lock->Invalidate();
  }

private:
  ILoadersContext& context_;
  boost::shared_ptr<IViewport>             viewport_;
  boost::shared_ptr<DicomResourcesLoader>  dicomLoader_;
  boost::shared_ptr<SeriesFramesLoader>    framesLoader_;

  SdlSimpleViewerApplication(ILoadersContext& context,
                             boost::shared_ptr<IViewport> viewport) :
    context_(context),
    viewport_(viewport)
  {
  }

  void Handle(const SeriesFramesLoader::FrameLoadedMessage& message)
  {
    LOG(INFO) << "Frame decoded! "
      << message.GetImage().GetWidth() << "x" << message.GetImage().GetHeight()
      << " " << Orthanc::EnumerationToString(message.GetImage().GetFormat());

    std::auto_ptr<TextureBaseSceneLayer> layer(
      message.GetInstanceParameters().CreateTexture(message.GetImage()));
    layer->SetLinearInterpolation(true);

    {
      std::auto_ptr<IViewport::ILock> lock(viewport_->Lock());
      lock->GetController().GetScene().SetLayer(0, layer.release());
      lock->GetCompositor().FitContent(lock->GetController().GetScene());
      lock->Invalidate();
    }
  }

  void Handle(const DicomResourcesLoader::SuccessMessage& message)
  {
    if (message.GetResources()->GetSize() != 1)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    //message.GetResources()->GetResource(0).Print(stdout);

    {
      std::auto_ptr<ILoadersContext::ILock> lock(context_.Lock());
      SeriesFramesLoader::Factory f(*message.GetResources());

      framesLoader_ = boost::dynamic_pointer_cast<SeriesFramesLoader>(
        f.Create(*lock));
      
      Register<SeriesFramesLoader::FrameLoadedMessage>(
        *framesLoader_, &SdlSimpleViewerApplication::Handle);

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

};
