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


static void GLAPIENTRY
OpenGLMessageCallback(GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam)
{
  if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
      (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
      type, severity, message);
  }
}

bool g_stopApplication = false;

void Run(TrackerSampleApp* app)
{
  SdlOpenGLWindow window("Hello", 1024, 768);

  app->GetScene().FitContent(window.GetCanvasWidth(), window.GetCanvasHeight());

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(OpenGLMessageCallback, 0);

  OpenGLCompositor compositor(window, app->GetScene());
  compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
    FONT_SIZE, Orthanc::Encoding_Latin1);

  while (!g_stopApplication)
  {
    compositor.Refresh();

    SDL_Event event;
    while (!g_stopApplication && SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        g_stopApplication = true;
        break;
      }
      else if (event.type == SDL_WINDOWEVENT &&
        event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      {
        app->DisableTracker(); // was: tracker.reset(NULL);
        compositor.UpdateSize();
      }
      else if (event.type == SDL_KEYDOWN &&
        event.key.repeat == 0 /* Ignore key bounce */)
      {
        switch (event.key.keysym.sym)
        {
        case SDLK_f:
          window.GetWindow().ToggleMaximize();
          break;

        case SDLK_q:
          g_stopApplication = true;
          break;
        default:
          break;
        }
      }
      app->HandleApplicationEvent(compositor, event);
    }
    SDL_Delay(1);
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
    TrackerSampleApp app(broker);
    app.PrepareScene();
    Run(&app);
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  StoneFinalize();

  return 0;
}
