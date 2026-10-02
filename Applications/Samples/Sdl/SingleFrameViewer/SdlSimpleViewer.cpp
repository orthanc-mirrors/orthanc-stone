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


#include "../../../../OrthancStone/Sources/Messages/TypedObserver.h"

#include "../../../../OrthancStone/Sources/Loaders/DicomResourcesLoader.h"
#include "../../../../OrthancStone/Sources/Loaders/SeriesFramesLoader.h"
#include "../../../../OrthancStone/Sources/Scene2D/AnnotationsSceneLayer.h"
#include "../../../../OrthancStone/Sources/Scene2DViewport/UndoStack.h"
#include "../../../../OrthancStone/Sources/Scene2DViewport/ViewportController.h"
#include "../../../../OrthancStone/Sources/StoneException.h"
#include "../../../../OrthancStone/Sources/StoneInitialization.h"
#include "../../../../OrthancStone/Sources/Viewport/DefaultViewportInteractor.h"
#include "../../Common/SampleHelpers.h"
#include "../SdlHelpers.h"

#include <Compatibility.h>  // For std::unique_ptr<>
#include <EmbeddedResources.h>

#include <SDL.h>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <string>



// TODO Refactoring
#include "../../../../OrthancStone/Sources/Oracle/ParseDicomFromFileCommand.h"
#include "../../../../OrthancStone/Sources/Oracle/SleepOracleCommand.h"
#include "../../../../OrthancStone/Sources/StoneApplication.h"


class Toto : public OrthancStone::IOracleClient
{
public:
  virtual void HandleSuccessFromOracle(const OrthancStone::IOracleCommand& command,
                                       const OrthancStone::IMessage& result)
  {
    // TODO Refactoring - Check that at this point, the environment mutex is locked
    LOG(ERROR) << "success!";

    if (command.GetType() == OrthancStone::IOracleCommand::Type_Http)
    {
      const OrthancStone::HttpCommand::SuccessMessage& m = dynamic_cast<const OrthancStone::HttpCommand::SuccessMessage&>(result);
      LOG(WARNING) << m.GetAnswer();
    }

    if (command.GetType() == OrthancStone::IOracleCommand::Type_OrthancRestApi)
    {
      const OrthancStone::OrthancRestApiCommand::SuccessMessage& m = dynamic_cast<const OrthancStone::OrthancRestApiCommand::SuccessMessage&>(result);
      LOG(WARNING) << m.GetAnswer();
    }
  }

  virtual void HandleErrorFromOracle(const OrthancStone::IOracleCommand& command,
                                     const Orthanc::OrthancException& error)
  {
    LOG(ERROR) << "error!";
  }
};
// END TODO Refactoring


namespace OrthancStone
{
  class ISingleViewportApplicationCore : public boost::noncopyable
  {
  public:
    virtual ~ISingleViewportApplicationCore()
    {
    }

    virtual void CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                  const boost::shared_ptr<IViewport>& viewport) = 0;

    virtual bool HandleKeyDown(char key) = 0;

    virtual void HandleMouseDown(IViewport::ILock& lock,
                                 const PointerEvent& p) = 0;

    virtual bool HandleMouseMove(IViewport::ILock& lock,
                                 const PointerEvent& p) = 0;

    virtual void Render(IViewport::ILock& lock) = 0;
  };


  class SingleViewportSdlApplication : public StoneApplication
  {
  private:
    boost::shared_ptr<ISingleViewportApplicationCore>  core_;
    boost::shared_ptr<SdlViewport>                     viewport_;

  protected:
    virtual void RunInternal(const boost::shared_ptr<Context>& context)
    {
      assert(core_.get() != NULL);
      core_->CreateComponents(context, viewport_);

      int scancodeCount = 0;
      const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);

      // SDL event loop
      bool stop = false;
      while (!stop)
      {
        bool paint = false;
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
          if (event.type == SDL_QUIT)
          {
            stop = true;
            break;
          }
          else if (viewport_->IsRefreshEvent(event))
          {
            paint = true;
          }
          else if (event.type == SDL_WINDOWEVENT &&
                   (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
          {
            viewport_->UpdateSize(event.window.data1, event.window.data2);
          }
          else if (event.type == SDL_WINDOWEVENT &&
                   (event.window.event == SDL_WINDOWEVENT_SHOWN ||
                    event.window.event == SDL_WINDOWEVENT_EXPOSED))
          {
            std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
            lock->RefreshCanvasSize();
          }
          else if (event.type == SDL_TEXTINPUT)
          {
            std::string s(event.text.text);
            if (s.size() == 1 &&
                s[0] > 0x00 &&
                s[0] <= 0x7f)
            {
              if (s[0] == 'f')
              {
                viewport_->ToggleMaximize();
              }
              else if (s[0] == 'q')
              {
                stop = true;
              }
              else
              {
                paint = core_->HandleKeyDown(s[0]);
              }
            }
          }
          else if (event.type == SDL_MOUSEBUTTONDOWN ||
                   event.type == SDL_MOUSEMOTION ||
                   event.type == SDL_MOUSEBUTTONUP)
          {
            std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());

            if (lock->HasCompositor())
            {
              PointerEvent p;
              OrthancStoneHelpers::GetPointerEvent(p, lock->GetCompositor(), event, keyboardState, scancodeCount);

              switch (event.type)
              {
                case SDL_MOUSEBUTTONDOWN:
                  core_->HandleMouseDown(*lock, p);
                  break;

                case SDL_MOUSEMOTION:
                  paint = core_->HandleMouseMove(*lock, p);
                  break;

                case SDL_MOUSEBUTTONUP:
                  lock->GetController().HandleMouseRelease(p);
                  lock->Invalidate();
                  break;

                default:
                  throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
              }
            }
          }
        }

        if (paint)
        {
          {
            std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
            core_->Render(*lock);
          }

          viewport_->Paint();
        }

        // Small delay to avoid using 100% of CPU
        SDL_Delay(1);
      }
    }

  public:
    SingleViewportSdlApplication(const Configuration& configuration,
                                 const boost::shared_ptr<ISingleViewportApplicationCore>& core,
                                 const std::string& title,
                                 unsigned int width,
                                 unsigned int height,
                                 bool useOpenGL) :
      StoneApplication(configuration),
      core_(core)
    {
      if (!core)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      if (useOpenGL)
      {
        viewport_ = SdlOpenGLViewport::Create(title, width, height);
      }
      else
      {
        viewport_ = SdlCairoViewport::Create(title, width, height);
      }

      std::string font;
      Orthanc::EmbeddedResources::GetFileResource(font, Orthanc::EmbeddedResources::UBUNTU_FONT);

      {
        std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
        lock->GetCompositor().SetFont(0, font, 16, Orthanc::Encoding_Latin1);
      }
    }
  };


  class SimpleViewerApp : public ISingleViewportApplicationCore,
                          public boost::enable_shared_from_this<SimpleViewerApp>,
                          public New::TypedObserver<FrameDecodedMessage>,
                          public New::TypedObserver<DicomResourcesLoadedMessage>
  {
  private:
    std::string                                   instanceId_;
    unsigned int                                  frameIndex_;
    boost::shared_ptr<StoneApplication::Context>  context_;
    boost::shared_ptr<IViewport>                  viewport_;
    AnnotationsSceneLayer                         annotations_;
    DefaultViewportInteractor                     interactor_;
    boost::shared_ptr<Toto>                       toto_;
    boost::shared_ptr<DicomResourcesLoader>       dicomLoader_;
    boost::shared_ptr<SeriesFramesLoader>         framesLoader_;
    Units                                         units_;

  public:
    virtual void Handle(const New::IObservable& observable,
                        const FrameDecodedMessage& message) ORTHANC_OVERRIDE
    {
      LOG(INFO) << "Frame decoded! "
                << message.GetImage().GetWidth() << "x" << message.GetImage().GetHeight()
                << " " << Orthanc::EnumerationToString(message.GetImage().GetFormat());

      std::unique_ptr<TextureBaseSceneLayer> layer(
        message.GetInstanceParameters().CreateTexture(message.GetImage()));
      //layer->SetLinearInterpolation(true);
      layer->SetLinearInterpolation(false);

      {
        std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
        lock->GetController().GetScene().SetLayer(0, layer.release());
        lock->GetCompositor().FitContent(lock->GetController().GetScene());
        lock->Invalidate();
      }
    }

    virtual void Handle(const New::IObservable& observable,
                        const DicomResourcesLoadedMessage& message) ORTHANC_OVERRIDE
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
        std::unique_ptr<IEnvironment::ILock> lock(context_->GetEnvironment().AcquireLock());

        framesLoader_ = SeriesFramesLoader::Create(*context_, *message.GetResources());

        //Register<FrameDecodedMessage>(*framesLoader_, &SimpleViewerApp::Handle);

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

  protected:
    virtual void CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                  const boost::shared_ptr<IViewport>& viewport) ORTHANC_OVERRIDE
    {
      context_ = context;
      viewport_ = viewport;

      dicomLoader_ = DicomResourcesLoader::Create(*context);

      //Register<DicomResourcesLoadedMessage>(*dicomLoader_, &SimpleViewerApp::Handle);

      //dicomLoader_->TypedObservable<DicomResourcesLoadedMessage>::Register(shared_from_this());
      New::IObservable::Bind<DicomResourcesLoadedMessage>(dicomLoader_, shared_from_this());

      annotations_.SetActiveTool(AnnotationsSceneLayer::Tool_Edit);
      annotations_.SetProbedLayer(0);

      interactor_.SetWindowingLayer(0);

      toto_.reset(new Toto);

      {
        std::unique_ptr<IViewport::ILock> lock(viewport->Lock());
        lock->GetController().SetUndoStack(boost::make_shared<UndoStack>());
      }

      {
        // std::unique_ptr<IEnvironment::ILock> lock(context->GetEnvironment().AcquireLock());
        DicomSource source;
        dicomLoader_->ScheduleLoadOrthancResource(boost::make_shared<LoadedDicomResources>(Orthanc::DICOM_TAG_SOP_INSTANCE_UID),
                                                  0, source, Orthanc::ResourceType_Instance, instanceId_,
                                                  new Orthanc::SingleValueObject<unsigned int>(frameIndex_));
      }
    }

    virtual bool HandleKeyDown(char key) ORTHANC_OVERRIDE
    {
      switch (key)
      {
        case 'b':
        {
          // TODO Refactoring
          IEnvironment& environment = context_->GetEnvironment();
          IOracle& oracle = context_->GetOracle();

          oracle.Submit(environment, toto_, new SleepOracleCommand(1000));

          {
            std::unique_ptr<HttpCommand> command(new HttpCommand);
            command->SetUrl("http://ip-api.com/json/");
            oracle.Submit(environment, toto_, command.release());
          }

          {
            std::unique_ptr<OrthancRestApiCommand> command(new OrthancRestApiCommand);
            command->SetUri("/system/");
            oracle.Submit(environment, toto_, command.release());
          }

          for (unsigned int i = 0; i < 10; i++)
          {
            DicomSource source;
            source.SetDicomDirSource();
            oracle.Submit(environment, toto_, new ParseDicomFromFileCommand(source, "hand.dcm"));
          }
          break;
        }

        case 's':
        {
          std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
          lock->GetCompositor().FitContent(lock->GetController().GetScene());
          lock->Invalidate();
          break;
        }

        case 'u':
        {
          std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
          if (lock->GetController().CanUndo())
          {
            lock->GetController().Undo();
          }
          break;
        }

        case 'U':
        {
          std::unique_ptr<IViewport::ILock> lock(viewport_->Lock());
          if (lock->GetController().CanRedo())
          {
            lock->GetController().Redo();
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

    virtual void HandleMouseDown(IViewport::ILock& lock,
                                 const PointerEvent& p) ORTHANC_OVERRIDE
    {
      annotations_.SetUnits(units_);

      boost::shared_ptr<IFlexiblePointerTracker> t;

      if (p.GetMouseButton() == MouseButton_Left)
      {
        t.reset(annotations_.CreateTracker(p.GetMainPosition(), lock.GetController().GetScene()));
      }

      if (t.get() != NULL)
      {
        lock.GetController().AcquireActiveTracker(t);
      }
      else
      {
        lock.GetController().HandleMousePress(interactor_, p,
                                              lock.GetCompositor().GetCanvasWidth(),
                                              lock.GetCompositor().GetCanvasHeight());
      }

      lock.Invalidate();
    }

    virtual bool HandleMouseMove(IViewport::ILock& lock,
                                 const PointerEvent& p) ORTHANC_OVERRIDE
    {
      if (lock.GetController().HandleMouseMove(p))
      {
        lock.Invalidate();
        if (annotations_.ClearHover())
        {
          return true;
        }
      }
      else
      {
        if (annotations_.SetMouseHover(p.GetMainPosition(), lock.GetController().GetScene()))
        {
          return true;
        }
      }

      return false;
    }

    virtual void Render(IViewport::ILock& lock)
    {
      annotations_.Render(lock.GetController().GetScene());
    }

  public:
    SimpleViewerApp(const std::string& instanceId,
                    unsigned int frameIndex) :
      instanceId_(instanceId),
      frameIndex_(frameIndex),
      annotations_(10),
      units_(Units_Pixels)
    {
    }
  };
}



static std::string orthancUrl;
static std::string instanceId;
static int frameIndex = 0;

static void ProcessOptions(int argc, char* argv[])
{
  namespace po = boost::program_options;
  po::options_description desc("Usage");

  desc.add_options()
    ("loglevel", po::value<std::string>()->default_value("WARNING"),
     "You can choose WARNING, INFO or TRACE for the logging level: Errors and warnings will always be displayed. (default: WARNING)")

    ("orthanc", po::value<std::string>()->default_value("http://localhost:8042"),
     "Base URL of the Orthanc instance")

    ("instance", po::value<std::string>()->default_value("285dece8-e1956b38-cdc7d084-6ce3371e-536a9ffc"),
     "Orthanc ID of the instance to display")

    ("frame_index", po::value<int>()->default_value(0),
     "The zero-based index of the frame (for multi-frame instances)")
    ;

  std::cout << desc << std::endl;

  std::cout << std::endl << "Keyboard shorcuts:" << std::endl
#if SAMPLE_USE_ANNOTATIONS_LAYER == 1
            << "  a\tCreate angle annotations" << std::endl
            << "  c\tCreate circle annotations" << std::endl
            << "  d\tDelete mode for annotations" << std::endl
            << "  e\tCreate ellipse probe" << std::endl
            << "  l\tCreate length annotations" << std::endl
            << "  m\tModification/edit mode, don't create annotation (default)" << std::endl
            << "  p\tCreate pixel probe" << std::endl
            << "  r\tCreate rectangle probe" << std::endl
#else
            << "  a\tEnable/disable the angle annotation tool" << std::endl
            << "  l\tEnable/disable the line annotation tool" << std::endl
            << "  r\tRedo the last edit to the annotation tools" << std::endl
            << "  u\tUndo the last edit to the annotation tools" << std::endl
#endif
            << "  f\tToggle fullscreen display" << std::endl
            << "  q\tExit" << std::endl
            << "  s\tFit the viewpoint to the image" << std::endl
            << std::endl << "Mouse buttons:" << std::endl
            << "  left  \tChange windowing, or edit annotation" << std::endl
            << "  center\tMove the viewpoint, or edit annotation" << std::endl
            << "  right \tZoom, or edit annotation" << std::endl
            << std::endl;

  po::variables_map vm;
  try
  {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  }
  catch (std::exception& e)
  {
    std::cerr << "Please check your command line options! (\"" << e.what() << "\")" << std::endl;
  }

  if (vm.count("loglevel") > 0)
  {
    std::string logLevel = vm["loglevel"].as<std::string>();
    OrthancStoneHelpers::SetLogLevel(logLevel);
  }

  if (vm.count("orthanc") > 0)
  {
    // maybe check URL validity here
    orthancUrl = vm["orthanc"].as<std::string>();
  }

  if (vm.count("instance") > 0)
  {
    instanceId = vm["instance"].as<std::string>();
  }

  if (vm.count("frame_index") > 0)
  {
    frameIndex = vm["frame_index"].as<int>();
  }
}


/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  int status = 0;

  OrthancStone::StoneInitialize();
  OrthancStone::SdlWindow::GlobalInitialize();

  try
  {
    ProcessOptions(argc, argv);

    //Orthanc::Logging::EnableInfoLevel(true);
    //Orthanc::Logging::EnableTraceLevel(true);

    Orthanc::WebServiceParameters orthancWebService;
    orthancWebService.SetUrl(orthancUrl);

    OrthancStone::StoneApplication::Configuration configuration;
    configuration.SetRemoteOrthancParameters(orthancWebService);
    configuration.SetDicomCacheSize(128 * 1024 * 1024);  // TODO Refactoring - Remove this
    configuration.SetRootDirectory("/tmp");  // TODO Refactoring - Remove this

    boost::shared_ptr<OrthancStone::SimpleViewerApp> core(new OrthancStone::SimpleViewerApp(instanceId, frameIndex));

    OrthancStone::SingleViewportSdlApplication app(
      configuration, core, "Stone of Orthanc", 800, 600, true /* use OpenGL */);

    app.Run();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "OrthancException: " << e.What();
    status = -1;
  }
  catch (OrthancStone::StoneException& e)
  {
    LOG(ERROR) << "StoneException: " << e.What();
    status = -1;
  }
  catch (std::runtime_error& e)
  {
    LOG(ERROR) << "Runtime error: " << e.what();
    status = -1;
  }
  catch (...)
  {
    LOG(ERROR) << "Native exception";
    status = -1;
  }

  OrthancStone::SdlWindow::GlobalFinalize();
  OrthancStone::StoneFinalize();

  return status;
}
