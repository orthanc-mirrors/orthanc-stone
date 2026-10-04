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


#include "SingleViewportSdlApplication.h"

#include "../../../OrthancStone/Sources/Scene2DViewport/ViewportController.h"
#include "../../../OrthancStone/Sources/StoneException.h"
#include "../Common/SampleHelpers.h"
#include "SdlHelpers.h"

#include <EmbeddedResources.h>  // For Orthanc::EmbeddedResources::UBUNTU_FONT


namespace OrthancStone
{
  void SingleViewportSdlApplication::RunInternal(const boost::shared_ptr<Context>& context)
  {
    assert(core_.get() != NULL);
    core_->CreateComponents(context, viewport_);

    std::unique_ptr<IViewportInteractor> interactor(core_->CreateMouseInteractor());
    if (interactor.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (startup_.get() != NULL)
    {
      startup_->Start(*core_);
    }

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
          std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());
          viewportLock->RefreshCanvasSize();
        }
        else if (event.type == SDL_TEXTINPUT)
        {
          std::string s(event.text.text);
          if (s.size() == 1 &&
              s[0] >= 32 &&
              s[0] <= 126)
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
              std::unique_ptr<IEnvironment::ILock> environmentLock(context->GetEnvironment().AcquireLock());
              core_->HandleKeyDown(*environmentLock, s[0]);
            }
          }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN ||
                 event.type == SDL_MOUSEMOTION ||
                 event.type == SDL_MOUSEBUTTONUP)
        {
          std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());

          if (viewportLock->HasCompositor())
          {
            PointerEvent p;
            OrthancStoneHelpers::GetPointerEvent(p, viewportLock->GetCompositor(), event, keyboardState, scancodeCount);

            switch (event.type)
            {
            case SDL_MOUSEBUTTONDOWN:
            {
              viewportLock->GetController().HandleMousePress(*interactor, p, viewportLock->GetCompositor().GetCanvasWidth(),
                                                             viewportLock->GetCompositor().GetCanvasHeight());
              viewportLock->Invalidate();
              break;
            }

            case SDL_MOUSEMOTION:
            {
              if (viewportLock->GetController().HasActiveTracker())
              {
                if (viewportLock->GetController().HandleMouseMove(p))
                {
                  viewportLock->Invalidate();
                }
              }
              else if (interactor->HasMouseHover())
              {
                interactor->HandleMouseHover(*viewport_, p);
                viewportLock->Invalidate();
              }
              break;
            }

            case SDL_MOUSEBUTTONUP:
              viewportLock->GetController().HandleMouseRelease(p);
              viewportLock->Invalidate();
              break;

            default:
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }
          }
        }
      }

      if (paint)
      {
        viewport_->Paint();
      }

      // Small delay to avoid using 100% of CPU
      SDL_Delay(1);
    }
  }


  SingleViewportSdlApplication::SingleViewportSdlApplication(const Configuration& configuration,
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
      std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());
      viewportLock->GetCompositor().SetFont(0, font, 16, Orthanc::Encoding_Latin1);
    }
  }


  void SingleViewportSdlApplication::SetCoreStartup(ICoreStartup* startup)
  {
    if (startup == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      startup_.reset(startup);
    }
  }


  bool SingleViewportSdlApplication::Run()
  {
    try
    {
      Start();
      RunInternal(GetContext());
      Stop();
      return true;
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "OrthancException: " << e.What();
      return false;
    }
    catch (StoneException& e)
    {
      LOG(ERROR) << "StoneException: " << e.What();
      return false;
    }
    catch (std::runtime_error& e)
    {
      LOG(ERROR) << "Runtime error: " << e.what();
      return false;
    }
    catch (...)
    {
      LOG(ERROR) << "Native exception";
      return false;
    }
  }
}
