#include "SingleFrameViewerApplication.h"

#include <Framework/Loaders/WebAssemblyLoadersContext.h>

#include <Framework/StoneException.h>
#include <Framework/StoneInitialization.h>

#include <Core/Toolbox.h>

#include <emscripten.h>
#include <emscripten/html5.h>


#define DISPATCH_JAVASCRIPT_EVENT(name)                         \
  EM_ASM(                                                       \
    const customEvent = document.createEvent("CustomEvent");    \
    customEvent.initCustomEvent(name, false, false, undefined); \
    window.dispatchEvent(customEvent);                          \
    );

#define EXTERN_CATCH_EXCEPTIONS                         \
  catch (Orthanc::OrthancException& e)                  \
  {                                                     \
    LOG(ERROR) << "OrthancException: " << e.What();     \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (OrthancStone::StoneException& e)               \
  {                                                     \
    LOG(ERROR) << "StoneException: " << e.What();       \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (std::exception& e)                             \
  {                                                     \
    LOG(ERROR) << "Runtime error: " << e.what();        \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (...)                                           \
  {                                                     \
    LOG(ERROR) << "Native exception";                   \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }



namespace OrthancStone
{
  class Observer : public IWebViewerLoadersObserver
  {
  public:
    virtual void SignalSeriesUpdated(LoadedDicomResources& series)
    {
      LOG(INFO) << "=====================================";
      LOG(INFO) << series.GetSize() << " series";

      /*for (size_t i = 0; i < series.GetSize(); i++)
        {
        series.GetResource(i).Print(stdout);
        printf("\n");
        }*/
    }
    
    virtual void SignalThumbnailLoaded(const std::string& studyInstanceUid,
                                       const std::string& seriesInstanceUid,
                                       SeriesThumbnailType type)
    {
      LOG(INFO) << "*** Thumbnail loaded: " << studyInstanceUid << " / "
                << seriesInstanceUid << " (type " << type << ")";
    }
  };
}



static std::auto_ptr<OrthancStone::WebAssemblyLoadersContext>  context_;
static boost::shared_ptr<OrthancStone::Application>  application_;


extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    try
    {
      Orthanc::Logging::Initialize();
      Orthanc::Logging::EnableInfoLevel(true);
      //Orthanc::Logging::EnableTraceLevel(true);
      LOG(WARNING) << "Initializing native Stone";

      LOG(WARNING) << "Compiled with Emscripten " << __EMSCRIPTEN_major__
                   << "." << __EMSCRIPTEN_minor__
                   << "." << __EMSCRIPTEN_tiny__;

      LOG(INFO) << "Endianness: " << Orthanc::EnumerationToString(Orthanc::Toolbox::DetectEndianness());
      context_.reset(new OrthancStone::WebAssemblyLoadersContext(1, 4, 1));
      context_->SetLocalOrthanc("..");
      context_->SetDicomCacheSize(128 * 1024 * 1024);  // 128MB
  
      DISPATCH_JAVASCRIPT_EVENT("StoneInitialized");
    }
    EXTERN_CATCH_EXCEPTIONS;

    return 0;
  }

  
  EMSCRIPTEN_KEEPALIVE
  void InitializeViewport(const char* canvasId)
  {
    try
    {
      if (context_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                        "The loaders context is not available yet");
      }
      
      if (application_.get() != NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                        "Only one single viewport is available for this application");
      }

      {
        std::auto_ptr<OrthancStone::Observer> observer(new OrthancStone::Observer);
      
#if 1
        OrthancStone::DicomSource source1;
        //source1.SetDicomWebSource("http://localhost:8042/dicom-web");  source1.SetDicomWebRendered(true);
        source1.SetDicomWebThroughOrthancSource("self");  source1.SetDicomWebRendered(true);
        //source1.SetDicomWebThroughOrthancSource("my-google");  source1.SetDicomWebRendered(false);
        boost::shared_ptr<OrthancStone::WebViewerLoaders> app2(
          OrthancStone::WebViewerLoaders::Create(*context_, source1, true, observer.release()));
#else
        OrthancStone::DicomSource source1;
        source1.SetOrthancSource();
        boost::shared_ptr<OrthancStone::WebViewerLoaders> app2(
          OrthancStone::WebViewerLoaders::Create(*context_, source1, true, observer.release()));
        //app2->AddOrthancStudy("27f7126f-4f66fb14-03f4081b-f9341db2-53925988");
        //app2->AddOrthancSeries("1e2c125c-411b8e86-3f4fe68e-a7584dd3-c6da78f0");
#endif

        // BRAINIX
        //app2->AddDicomAllSeries();
        //app2->AddDicomStudy("2.16.840.1.113669.632.20.1211.10000357775");
        app2->AddDicomSeries("2.16.840.1.113669.632.20.1211.10000357775", "1.3.46.670589.11.0.0.11.4.2.0.8743.5.3800.2006120117110979000"); // Standard image: type 5

        app2->AddDicomStudy("1.3.51.0.7.633920140505.6339234439.633987.633918098");  // "Video" type 4: video720p.dcm
        app2->AddDicomStudy("1.2.276.0.7230010.3.1.2.2344313775.14992.1458058404.7528");  // "PDF" type 3: pdf.dcm  
        app2->AddDicomSeries("1.2.276.0.7230010.3.1.2.296485376.1.1568899779.944131", "1.2.276.0.7230010.3.1.3.296485376.1.1568899781.944588"); // RTSTRUCT, "Unsupported" type 2: DICOM/WebViewer2/TFE/IMAGES/IM452

      
        //app2->AddDicomStudy("1.2.276.0.7230010.3.1.2.380371456.1.1544616291.954997");  // CSPO
      }

      boost::shared_ptr<OrthancStone::WebGLViewport> viewport(OrthancStone::GetWebGLViewportsRegistry().Add(canvasId));
      application_ = OrthancStone::Application::Create(*context_, viewport);

      // Paint the viewport to black
      
      {
        OrthancStone::WebGLViewportsRegistry::Accessor accessor(
          OrthancStone::GetWebGLViewportsRegistry(), canvasId);

        if (accessor.IsValid())
        {
          accessor.GetViewport().Invalidate();
        }
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  
  EMSCRIPTEN_KEEPALIVE
  void LoadOrthanc(const char* instance,
                   int frame)
  {
    try
    {
      if (application_.get() != NULL)
      {
        OrthancStone::DicomSource source;
        application_->LoadOrthancFrame(source, instance, frame);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  
  EMSCRIPTEN_KEEPALIVE
  void LoadDicomWeb(const char* server,
                    const char* studyInstanceUid,
                    const char* seriesInstanceUid,
                    const char* sopInstanceUid,
                    int frame)
  {
    try
    {
      if (application_.get() != NULL)
      {
        OrthancStone::DicomSource source;
        source.SetDicomWebThroughOrthancSource(server);
        application_->LoadDicomWebFrame(source, studyInstanceUid, seriesInstanceUid,
                                        sopInstanceUid, frame);
      }
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
}
