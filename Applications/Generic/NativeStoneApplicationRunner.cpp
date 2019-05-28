/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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


#if ORTHANC_ENABLE_THREADS != 1
#error this file shall be included only with the ORTHANC_ENABLE_THREADS set to 1
#endif

#include "NativeStoneApplicationRunner.h"

#include "../../Framework/Deprecated/Toolbox/MessagingToolbox.h"
#include "../../Platforms/Generic/OracleWebService.h"
#include "../../Platforms/Generic/OracleDelayedCallExecutor.h"
#include "NativeStoneApplicationContext.h"

#include <Core/Logging.h>
#include <Core/HttpClient.h>
#include <Core/Toolbox.h>
#include <Core/OrthancException.h>
#include <Plugins/Samples/Common/OrthancHttpConnection.h>

#include <boost/program_options.hpp>

namespace OrthancStone
{
  // Anonymous namespace to avoid clashes against other compilation modules
  namespace
  {
    class LogStatusBar : public Deprecated::IStatusBar
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

  int NativeStoneApplicationRunner::Execute(int argc,
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

    { // generic options
      boost::program_options::options_description generic("Generic options");
      generic.add_options()
          ("help", "Display this help and exit")
          ("verbose", "Be verbose in logs")
          ("orthanc", boost::program_options::value<std::string>()->
            default_value("http://localhost:8042/"),
           "URL to the Orthanc server")
          ("username", "Username for the Orthanc server")
          ("password", "Password for the Orthanc server")
          ("https-verify", boost::program_options::value<bool>()->
            default_value(true), "Check HTTPS certificates")
          ;

      options.add(generic);
    }

    // platform specific options
    DeclareCommandLineOptions(options);
    
    // application specific options
    application_.DeclareStartupOptions(options);

    boost::program_options::variables_map parameters;
    bool error = false;

    try
    {
      boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv).
          options(options).allow_unregistered().run(), parameters);
      boost::program_options::notify(parameters);
    }
    catch (boost::program_options::error& e)
    {
      LOG(ERROR) << 
        "Error while parsing the command-line arguments: " << e.what();
      error = true;
    }


    /******************************************************************
     * Configure the application with the command-line parameters
     ******************************************************************/

    if (error || parameters.count("help"))
    {
      std::cout << std::endl;

      std::cout << options << "\n";
      return error ? -1 : 0;
    }

    if (parameters.count("https-verify") &&
        !parameters["https-verify"].as<bool>())
    {
      LOG(WARNING) << "Turning off verification of HTTPS certificates (unsafe)";
      Orthanc::HttpClient::ConfigureSsl(false, "");
    }

    LOG(ERROR) << "???????? if (parameters.count(\"verbose\"))";
    if (parameters.count("verbose"))
    {
      LOG(ERROR) << "parameters.count(\"verbose\") != 0";
      Orthanc::Logging::EnableInfoLevel(true);
      LOG(INFO) << "Verbose logs are enabled";
    }

    LOG(ERROR) << "???????? if (parameters.count(\"trace\"))";
    if (parameters.count("trace"))
    {
      LOG(ERROR) << "parameters.count(\"trace\") != 0";
      Orthanc::Logging::EnableTraceLevel(true);
      VLOG(1) << "Trace logs are enabled";
    }

    ParseCommandLineOptions(parameters);

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

      if (parameters.count("username") && parameters.count("password"))
      {
        webServiceParameters.SetCredentials(parameters["username"].
          as<std::string>(),
          parameters["password"].as<std::string>());
      }

      LOG(WARNING) << "URL to the Orthanc REST API: " << 
        webServiceParameters.GetUrl();

      {
        OrthancPlugins::OrthancHttpConnection orthanc(webServiceParameters);
        if (!Deprecated::MessagingToolbox::CheckOrthancVersion(orthanc))
        {
          LOG(ERROR) << "Your version of Orthanc is incompatible with Stone of "
            << "Orthanc, please upgrade";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
        }
      }


      /****************************************************************
       * Initialize the application
       ****************************************************************/

      LOG(WARNING) << "Creating the widgets of the application";

      LogStatusBar statusBar;

      NativeStoneApplicationContext context(broker_);

      {
        // use multiple threads to execute asynchronous tasks like 
        // download content
        Deprecated::Oracle oracle(6); 
        oracle.Start();

        {
          Deprecated::OracleWebService webService(
            broker_, oracle, webServiceParameters, context);
          
          context.SetWebService(webService);
          context.SetOrthancBaseUrl(webServiceParameters.GetUrl());

          Deprecated::OracleDelayedCallExecutor delayedExecutor(broker_, oracle, context);
          context.SetDelayedCallExecutor(delayedExecutor);

          application_.Initialize(&context, statusBar, parameters);

          {
            NativeStoneApplicationContext::GlobalMutexLocker locker(context);
            locker.SetCentralWidget(application_.GetCentralWidget());
            locker.GetCentralViewport().SetStatusBar(statusBar);
          }

          std::string title = application_.GetTitle();
          if (title.empty())
          {
            title = "Stone of Orthanc";
          }

          /****************************************************************
           * Run the application
           ****************************************************************/

          Run(context, title, argc, argv);

          /****************************************************************
           * Finalize the application
           ****************************************************************/

          oracle.Stop();
        }
      }

      LOG(WARNING) << "The application is stopping";
      application_.Finalize();
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
