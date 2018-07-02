/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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


#if ORTHANC_ENABLE_SDL != 1
#error this file shall be included only with the ORTHANC_ENABLE_SDL set to 1
#endif

#include "BasicSdlApplication.h"
#include <boost/program_options.hpp>

#include "../../Framework/Toolbox/MessagingToolbox.h"
#include "SdlEngine.h"

#include <Core/Logging.h>
#include <Core/HttpClient.h>
#include <Plugins/Samples/Common/OrthancHttpConnection.h>

namespace OrthancStone
{
  // Anonymous namespace to avoid clashes against other compilation modules
  namespace
  {
    class LogStatusBar : public IStatusBar
    {
    public:
      virtual void ClearMessage()
      {
      }

      virtual void SetMessage(const std::string& message)
      {
        LOG(WARNING) << message;
      }
    };
  }


  static void DeclareSdlCommandLineOptions(boost::program_options::options_description& options)
  {
    // Declare the supported parameters
    boost::program_options::options_description generic("Generic options");
    generic.add_options()
      ("help", "Display this help and exit")
      ("verbose", "Be verbose in logs")
      ("orthanc", boost::program_options::value<std::string>()->default_value("http://localhost:8042/"),
       "URL to the Orthanc server")
      ("username", "Username for the Orthanc server")
      ("password", "Password for the Orthanc server")
      ("https-verify", boost::program_options::value<bool>()->default_value(true), "Check HTTPS certificates")
      ;

    options.add(generic);

    boost::program_options::options_description sdl("SDL options");
    sdl.add_options()
      ("width", boost::program_options::value<int>()->default_value(1024), "Initial width of the SDL window")
      ("height", boost::program_options::value<int>()->default_value(768), "Initial height of the SDL window")
      ("opengl", boost::program_options::value<bool>()->default_value(true), "Enable OpenGL in SDL")
      ;

    options.add(sdl);
  }


  int BasicSdlApplication::ExecuteWithSdl(MessageBroker& broker,
                                          IBasicApplication& application,
                                        int argc,
                                        char* argv[])
  {
    /******************************************************************
     * Initialize all the subcomponents of Orthanc Stone
     ******************************************************************/

    Orthanc::Logging::Initialize();
    Orthanc::HttpClient::InitializeOpenSsl();
    Orthanc::HttpClient::GlobalInitialize();
    SdlWindow::GlobalInitialize();


    /******************************************************************
     * Declare and parse the command-line options of the application
     ******************************************************************/

    boost::program_options::options_description options;
    DeclareSdlCommandLineOptions(options);
    application.DeclareStartupOptions(options);

    boost::program_options::variables_map parameters;
    bool error = false;

    try
    {
      boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
                                    options(options).run(), parameters);
      boost::program_options::notify(parameters);
    }
    catch (boost::program_options::error& e)
    {
      LOG(ERROR) << "Error while parsing the command-line arguments: " << e.what();
      error = true;
    }


    /******************************************************************
     * Configure the application with the command-line parameters
     ******************************************************************/

    if (error || parameters.count("help"))
    {
      std::cout << std::endl
                << "Usage: " << argv[0] << " [OPTION]..."
                << std::endl
                << "Orthanc, lightweight, RESTful DICOM server for healthcare and medical research."
                << std::endl << std::endl
                << "Demonstration application of Orthanc Stone using SDL."
                << std::endl;

      std::cout << options << "\n";
      return error ? -1 : 0;
    }

    if (parameters.count("https-verify") &&
        !parameters["https-verify"].as<bool>())
    {
      LOG(WARNING) << "Turning off verification of HTTPS certificates (unsafe)";
      Orthanc::HttpClient::ConfigureSsl(false, "");
    }

    if (parameters.count("verbose"))
    {
      Orthanc::Logging::EnableInfoLevel(true);
    }

    if (!parameters.count("width") ||
        !parameters.count("height") ||
        !parameters.count("opengl"))
    {
      LOG(ERROR) << "Parameter \"width\", \"height\" or \"opengl\" is missing";
      return -1;
    }

    int w = parameters["width"].as<int>();
    int h = parameters["height"].as<int>();
    if (w <= 0 || h <= 0)
    {
      LOG(ERROR) << "Parameters \"width\" and \"height\" must be positive";
      return -1;
    }

    unsigned int width = static_cast<unsigned int>(w);
    unsigned int height = static_cast<unsigned int>(h);
    LOG(WARNING) << "Initial display size: " << width << "x" << height;

    bool opengl = parameters["opengl"].as<bool>();
    if (opengl)
    {
      LOG(WARNING) << "OpenGL is enabled, disable it with option \"--opengl=off\" if the application crashes";
    }
    else
    {
      LOG(WARNING) << "OpenGL is disabled, enable it with option \"--opengl=on\" for best performance";
    }

    bool success = true;
    try
    {
      /****************************************************************
       * Initialize the connection to the Orthanc server
       ****************************************************************/

      Orthanc::WebServiceParameters webServiceParameters;

      if (parameters.count("orthanc"))
      {
        webServiceParameters.SetUrl(parameters["orthanc"].as<std::string>());
      }

      if (parameters.count("username"))
      {
        webServiceParameters.SetUsername(parameters["username"].as<std::string>());
      }

      if (parameters.count("password"))
      {
        webServiceParameters.SetPassword(parameters["password"].as<std::string>());
      }

      LOG(WARNING) << "URL to the Orthanc REST API: " << webServiceParameters.GetUrl();

      {
        OrthancPlugins::OrthancHttpConnection orthanc(webServiceParameters);
        if (!MessagingToolbox::CheckOrthancVersion(orthanc))
        {
          LOG(ERROR) << "Your version of Orthanc is incompatible with Stone of Orthanc, please upgrade";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }
      }


      /****************************************************************
       * Initialize the application
       ****************************************************************/

      LOG(WARNING) << "Creating the widgets of the application";

      LogStatusBar statusBar;

      boost::mutex stoneGlobalMutex;
      Oracle oracle(stoneGlobalMutex, 4); // use 4 threads to download content
      OracleWebService webService(broker, oracle, webServiceParameters);
      BasicSdlApplicationContext context(webService);

      application.Initialize(&context, statusBar, parameters);

      {
        BasicSdlApplicationContext::ViewportLocker locker(context);
        context.SetCentralWidget(application.GetCentralWidget());
        locker.GetViewport().SetStatusBar(statusBar);
      }

      std::string title = application.GetTitle();
      if (title.empty())
      {
        title = "Stone of Orthanc";
      }

      {
        /**************************************************************
         * Run the application inside a SDL window
         **************************************************************/

        LOG(WARNING) << "Starting the application";

        SdlWindow window(title.c_str(), width, height, opengl);
        SdlEngine sdl(window, context);

        {
          BasicSdlApplicationContext::ViewportLocker locker(context);
          locker.GetViewport().Register(sdl);  // (*)
        }

        context.Start();
        sdl.Run();

        LOG(WARNING) << "Stopping the application";

        // Don't move the "Stop()" command below out of the block,
        // otherwise the application might crash, because the
        // "SdlEngine" is an observer of the viewport (*) and the
        // update thread started by "context.Start()" would call a
        // destructed object (the "SdlEngine" is deleted with the
        // lexical scope).
        context.Stop();
      }


      /****************************************************************
       * Finalize the application
       ****************************************************************/

      LOG(WARNING) << "The application has stopped";
      application.Finalize();
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "EXCEPTION: " << e.What();
      success = false;
    }


    /******************************************************************
     * Finalize all the subcomponents of Orthanc Stone
     ******************************************************************/

    SdlWindow::GlobalFinalize();
    Orthanc::HttpClient::GlobalFinalize();
    Orthanc::HttpClient::FinalizeOpenSsl();

    return (success ? 0 : -1);
  }

}
