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


#include "../../Common/SimpleViewerCore.h"
#include "../../SingleViewportSdlApplication.h"

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


namespace OrthancStone
{
  class Toto : public IOracleClient
  {
  public:
    virtual void HandleSuccessFromOracle(const IOracleCommand& command,
                                         const IMessage& result)
    {
      // TODO Refactoring - Check that at this point, the environment mutex is locked
      LOG(ERROR) << "success!";

      if (command.GetType() == IOracleCommand::Type_Http)
      {
        const HttpCommand::SuccessMessage& m = dynamic_cast<const HttpCommand::SuccessMessage&>(result);
        LOG(WARNING) << m.GetAnswer();
      }

      if (command.GetType() == IOracleCommand::Type_OrthancRestApi)
      {
        const OrthancRestApiCommand::SuccessMessage& m = dynamic_cast<const OrthancRestApiCommand::SuccessMessage&>(result);
        LOG(WARNING) << m.GetAnswer();
      }
    }

    virtual void HandleErrorFromOracle(const IOracleCommand& command,
                                       const Orthanc::OrthancException& error)
    {
      LOG(ERROR) << "error!";
    }
  };

  class Tutu : public SimpleViewerCore
  {
  private:
    boost::shared_ptr<Toto>  toto_;

  protected:
    virtual void CreateComponents(const boost::shared_ptr<StoneApplication::Context>& context,
                                  const boost::shared_ptr<IViewport>& viewport) ORTHANC_OVERRIDE
    {
      toto_.reset(new Toto);
      SimpleViewerCore::CreateComponents(context, viewport);
    }


    virtual bool HandleKeyDown(const IEnvironment::ILock& environmentLock,
                               char key) ORTHANC_OVERRIDE
    {
      if (key == 'b')
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

        return false;
      }
      else
      {
        return SimpleViewerCore::HandleKeyDown(environmentLock, key);
      }
    }

  public:
    Tutu(const std::string& instanceId,
         unsigned int frameIndex) :
      SimpleViewerCore(instanceId, frameIndex)
    {
    }
  };
}
// END TODO Refactoring


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

    boost::shared_ptr<OrthancStone::SimpleViewerCore> core(new OrthancStone::Tutu(instanceId, frameIndex));

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
