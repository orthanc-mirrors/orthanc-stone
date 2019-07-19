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

#include "../Shared/RadiographyEditorApp.h"

// From Stone
#include "../../Framework/Oracle/SleepOracleCommand.h"
#include "../../Framework/Oracle/ThreadedOracle.h"
#include "../../Applications/Sdl/SdlOpenGLWindow.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
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

using namespace OrthancStone;

namespace OrthancStone
{
  class NativeApplicationContext : public IMessageEmitter
  {
  private:
    boost::shared_mutex  mutex_;
    MessageBroker        broker_;
    IObservable          oracleObservable_;

  public:
    NativeApplicationContext() :
      oracleObservable_(broker_)
    {
    }


    virtual void EmitMessage(const IObserver& observer,
                             const IMessage& message) ORTHANC_OVERRIDE
    {
      try
      {
        boost::unique_lock<boost::shared_mutex>  lock(mutex_);
        oracleObservable_.EmitMessage(observer, message);
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Exception while emitting a message: " << e.What();
      }
    }


    class ReaderLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::shared_lock<boost::shared_mutex>  lock_;

    public:
      ReaderLock(NativeApplicationContext& that) :
        that_(that),
        lock_(that.mutex_)
      {
      }
    };


    class WriterLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::unique_lock<boost::shared_mutex>  lock_;

    public:
      WriterLock(NativeApplicationContext& that) :
        that_(that),
        lock_(that.mutex_)
      {
      }

      MessageBroker& GetBroker()
      {
        return that_.broker_;
      }

      IObservable& GetOracleObservable()
      {
        return that_.oracleObservable_;
      }
    };
  };
}

class OpenGlSdlCompositorFactory : public ICompositorFactory
{
  OpenGL::IOpenGLContext& openGlContext_;

public:
  OpenGlSdlCompositorFactory(OpenGL::IOpenGLContext& openGlContext) :
    openGlContext_(openGlContext)
  {}

  ICompositor* GetCompositor(const Scene2D& scene)
  {

    OpenGLCompositor* compositor = new OpenGLCompositor(openGlContext_, scene);
    compositor->SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_0, Orthanc::Encoding_Latin1);
    compositor->SetFont(1, Orthanc::EmbeddedResources::UBUNTU_FONT,
                         FONT_SIZE_1, Orthanc::Encoding_Latin1);
    return compositor;
  }
};

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
  //  Orthanc::Logging::EnableTraceLevel(true);

  try
  {
    OrthancStone::NativeApplicationContext context;
    OrthancStone::NativeApplicationContext::WriterLock lock(context);
    OrthancStone::ThreadedOracle oracle(context);

    // False means we do NOT let Windows treat this as a legacy application
    // that needs to be scaled
    SdlOpenGLWindow window("Hello", 1024, 1024, false);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(OpenGLMessageCallback, 0);

    std::auto_ptr<OpenGlSdlCompositorFactory> compositorFactory(new OpenGlSdlCompositorFactory(window));
    boost::shared_ptr<RadiographyEditorApp> app(new RadiographyEditorApp(oracle, lock.GetOracleObservable(), compositorFactory.release()));
    app->PrepareScene();
    app->FitContent(window.GetCanvasWidth(), window.GetCanvasHeight());

    bool stopApplication = false;

    while (!stopApplication)
    {
      app->Refresh();

      SDL_Event event;
      while (!stopApplication && SDL_PollEvent(&event))
      {
        OrthancStone::KeyboardModifiers modifiers = OrthancStone::KeyboardModifiers_None;
        if (event.key.keysym.mod & KMOD_CTRL)
          modifiers = static_cast<OrthancStone::KeyboardModifiers>(static_cast<int>(modifiers) | static_cast<int>(OrthancStone::KeyboardModifiers_Control));
        if (event.key.keysym.mod & KMOD_ALT)
          modifiers = static_cast<OrthancStone::KeyboardModifiers>(static_cast<int>(modifiers) | static_cast<int>(OrthancStone::KeyboardModifiers_Alt));
        if (event.key.keysym.mod & KMOD_SHIFT)
          modifiers = static_cast<OrthancStone::KeyboardModifiers>(static_cast<int>(modifiers) | static_cast<int>(OrthancStone::KeyboardModifiers_Shift));

        OrthancStone::MouseButton button;
        if (event.button.button == SDL_BUTTON_LEFT)
          button = OrthancStone::MouseButton_Left;
        else if (event.button.button == SDL_BUTTON_MIDDLE)
          button = OrthancStone::MouseButton_Middle;
        else if (event.button.button == SDL_BUTTON_RIGHT)
          button = OrthancStone::MouseButton_Right;

        if (event.type == SDL_QUIT)
        {
          stopApplication = true;
          break;
        }
        else if (event.type == SDL_WINDOWEVENT &&
                 event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
          app->DisableTracker(); // was: tracker.reset(NULL);
          app->UpdateSize();
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
            stopApplication = true;
            break;
          default:
          {
            app->OnKeyPressed(event.key.keysym.sym, modifiers);
           }
          }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
          app->OnMouseDown(event.button.x, event.button.y, modifiers, button);
        }
        else if (event.type == SDL_MOUSEMOTION)
        {
          app->OnMouseMove(event.button.x, event.button.y, modifiers);
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
          app->OnMouseUp(event.button.x, event.button.y, modifiers, button);
        }
      }
      SDL_Delay(1);
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  StoneFinalize();

  return 0;
}


