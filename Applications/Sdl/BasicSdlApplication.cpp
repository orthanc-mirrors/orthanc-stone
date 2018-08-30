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
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/OrthancHttpConnection.h>
#include "../../Platforms/Generic/OracleWebService.h"

namespace OrthancStone
{
  void SdlStoneApplicationRunner::Initialize()
  {
    SdlWindow::GlobalInitialize();
  }

  void SdlStoneApplicationRunner::DeclareCommandLineOptions(boost::program_options::options_description& options)
  {
    boost::program_options::options_description sdl("SDL options");
    sdl.add_options()
        ("width", boost::program_options::value<int>()->default_value(1024), "Initial width of the SDL window")
        ("height", boost::program_options::value<int>()->default_value(768), "Initial height of the SDL window")
        ("opengl", boost::program_options::value<bool>()->default_value(true), "Enable OpenGL in SDL")
        ;

    options.add(sdl);
  }

  void SdlStoneApplicationRunner::ParseCommandLineOptions(const boost::program_options::variables_map& parameters)
  {
    if (!parameters.count("width") ||
        !parameters.count("height") ||
        !parameters.count("opengl"))
    {
      LOG(ERROR) << "Parameter \"width\", \"height\" or \"opengl\" is missing";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    int w = parameters["width"].as<int>();
    int h = parameters["height"].as<int>();
    if (w <= 0 || h <= 0)
    {
      LOG(ERROR) << "Parameters \"width\" and \"height\" must be positive";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    width_ = static_cast<unsigned int>(w);
    height_ = static_cast<unsigned int>(h);
    LOG(WARNING) << "Initial display size: " << width_ << "x" << height_;

    enableOpenGl_ = parameters["opengl"].as<bool>();
    if (enableOpenGl_)
    {
      LOG(WARNING) << "OpenGL is enabled, disable it with option \"--opengl=off\" if the application crashes";
    }
    else
    {
      LOG(WARNING) << "OpenGL is disabled, enable it with option \"--opengl=on\" for best performance";
    }

  }

  void SdlStoneApplicationRunner::Run(NativeStoneApplicationContext& context, const std::string& title, int argc, char* argv[])
  {
    /**************************************************************
     * Run the application inside a SDL window
     **************************************************************/

    LOG(WARNING) << "Starting the application";

    SdlWindow window(title.c_str(), width_, height_, enableOpenGl_);
    SdlEngine sdl(window, context);

    {
      NativeStoneApplicationContext::GlobalMutexLocker locker(context);
      context.GetCentralViewport().Register(sdl);  // (*)
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

  void SdlStoneApplicationRunner::Finalize()
  {
    SdlWindow::GlobalFinalize();
  }


}
