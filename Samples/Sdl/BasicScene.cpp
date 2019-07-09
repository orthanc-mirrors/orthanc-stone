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


// From Stone
#include "../../Applications/Sdl/SdlOpenGLWindow.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2DViewport/UndoStack.h"
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Messages/MessageBroker.h"

// From Orthanc framework
#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

#include <SDL.h>
#include <stdio.h>


#include "../Shared/SharedBasicScene.h"

using namespace OrthancStone;

boost::shared_ptr<BasicScene2DInteractor> interactor;

void HandleApplicationEvent(boost::shared_ptr<OrthancStone::ViewportController> controller,
                            const OrthancStone::OpenGLCompositor& compositor,
                            const SDL_Event& event)
{
  using namespace OrthancStone;
  Scene2D& scene(*controller->GetScene());
  if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION)
  {
    // TODO: this code is copy/pasted from GuiAdapter::Run() -> find the right place
    int scancodeCount = 0;
    const uint8_t* keyboardState = SDL_GetKeyboardState(&scancodeCount);
    bool ctrlPressed(false);
    bool shiftPressed(false);
    bool altPressed(false);

    if (SDL_SCANCODE_LCTRL < scancodeCount && keyboardState[SDL_SCANCODE_LCTRL])
      ctrlPressed = true;
    if (SDL_SCANCODE_RCTRL < scancodeCount && keyboardState[SDL_SCANCODE_RCTRL])
      ctrlPressed = true;
    if (SDL_SCANCODE_LSHIFT < scancodeCount && keyboardState[SDL_SCANCODE_LSHIFT])
      shiftPressed = true;
    if (SDL_SCANCODE_RSHIFT < scancodeCount && keyboardState[SDL_SCANCODE_RSHIFT])
      shiftPressed = true;
    if (SDL_SCANCODE_LALT < scancodeCount && keyboardState[SDL_SCANCODE_LALT])
      altPressed = true;

    GuiAdapterMouseEvent guiEvent;
    ConvertFromPlatform(guiEvent, ctrlPressed, shiftPressed, altPressed, event);
    PointerEvent pointerEvent;
    pointerEvent.AddPosition(compositor.GetPixelCenterCoordinates(event.button.x, event.button.y));

    interactor->OnMouseEvent(guiEvent, pointerEvent);
    return;
  }
  else if ((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) && event.key.repeat == 0  /* Ignore key bounce */)
  {
    GuiAdapterKeyboardEvent guiEvent;
    ConvertFromPlatform(guiEvent, event);

    interactor->OnKeyboardEvent(guiEvent);
  }

}


static void GLAPIENTRY
OpenGLMessageCallback(GLenum source,
                      GLenum type,
                      GLuint id,
                      GLenum severity,
                      GLsizei length,
                      const GLchar* message,
                      const void* userParam )
{
  if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
  }
}


void Run(boost::shared_ptr<OrthancStone::ViewportController> controller)
{
  SdlOpenGLWindow window("Hello", 1024, 768);

  controller->FitContent(window.GetCanvasWidth(), window.GetCanvasHeight());
  
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(OpenGLMessageCallback, 0);

  boost::shared_ptr<OpenGLCompositor> compositor(new OpenGLCompositor(window, *controller->GetScene()));
  compositor->SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
                     BASIC_SCENE_FONT_SIZE, Orthanc::Encoding_Latin1);
  interactor->SetCompositor(compositor);

  bool stop = false;
  while (!stop)
  {
    compositor->Refresh();

    SDL_Event event;
    while (!stop &&
           SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        stop = true;
        break;
      }
      else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      {
        compositor->UpdateSize();
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
            stop = true;
            break;

          default:
            break;
        }
      }
      
      HandleApplicationEvent(controller, *compositor, event);
    }

    SDL_Delay(1);
  }
  interactor.reset();
}




/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  using namespace OrthancStone;
  StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);

  try
  {
    MessageBroker broker;
    boost::shared_ptr<UndoStack> undoStack(new UndoStack);
    boost::shared_ptr<ViewportController> controller = boost::make_shared<ViewportController>(
      undoStack, boost::ref(broker));
    interactor.reset(new BasicScene2DInteractor(controller));
    PrepareScene(controller);
    Run(controller);
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  StoneFinalize();

  return 0;
}
