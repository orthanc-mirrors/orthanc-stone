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

#include "TrackerSampleApp.h"

 // From Stone
#include "../../Applications/Sdl/SdlOpenGLWindow.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/StoneInitialization.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>


#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <SDL.h>
#include <stdio.h>

/*
TODO:

- to decouple the trackers from the sample, we need to supply them with
  the scene rather than the app

- in order to do that, we need a GetNextFreeZIndex function (or something 
  along those lines) in the scene object

*/


using namespace Orthanc;
using namespace OrthancStone;




boost::weak_ptr<TrackerSampleApp> g_app;

void TrackerSample_SetInfoDisplayMessage(std::string key, std::string value)
{
  boost::shared_ptr<TrackerSampleApp> app = g_app.lock();
  if (app)
  {
    app->SetInfoDisplayMessage(key, value);
  }
}

/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);
  Orthanc::Logging::EnableTraceLevel(true);

  try
  {
    MessageBroker broker;
    boost::shared_ptr<TrackerSampleApp> app(new TrackerSampleApp(broker));
    g_app = app;
    app->PrepareScene();
    app->Run();
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  StoneFinalize();

  return 0;
}


