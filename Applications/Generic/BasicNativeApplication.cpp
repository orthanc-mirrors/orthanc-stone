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


#if ORTHANC_ENABLE_NATIVE != 1
#error this file shall be included only with the ORTHANC_ENABLE_NATIVE set to 1
#endif

#include "BasicNativeApplication.h"
#include "BasicNativeApplicationContext.h"
#include <boost/program_options.hpp>

#include "../../Framework/Toolbox/MessagingToolbox.h"

#include <Core/Logging.h>
#include <Core/HttpClient.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/OrthancHttpConnection.h>
#include "../../Platforms/Generic/OracleWebService.h"

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

  int BasicNativeApplication::Execute(MessageBroker& broker,
                                   IBasicApplication& application,
                                   int argc,
                                   char* argv[])
  {
    /******************************************************************
     * Initialize all the subcomponents of Orthanc Stone
     ******************************************************************/

    Orthanc::Logging::Initialize();
    Orthanc::Toolbox::InitializeOpenSsl();
    Orthanc::HttpClient::GlobalInitialize();

    Initialize();

    /******************************************************************
     * Declare and parse the command-line options of the application
     ******************************************************************/

    boost::program_options::options_description options;
    DeclareCommandLineOptions(options);
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

      BasicNativeApplicationContext context;
      Oracle oracle(4); // use 4 threads to download content
      OracleWebService webService(broker, oracle, webServiceParameters, context);
      context.SetWebService(webService);

      application.Initialize(&context, statusBar, parameters);

      {
        BasicNativeApplicationContext::GlobalMutexLocker locker(context);
        context.SetCentralWidget(application.GetCentralWidget());
        context.GetCentralViewport().SetStatusBar(statusBar);
      }

      std::string title = application.GetTitle();
      if (title.empty())
      {
        title = "Stone of Orthanc";
      }

      /****************************************************************
       * Run the application
       ****************************************************************/

      Run(context, title, width, height, opengl);


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

    Finalize();
    Orthanc::HttpClient::GlobalFinalize();
    Orthanc::Toolbox::FinalizeOpenSsl();

    return (success ? 0 : -1);
  }

}
