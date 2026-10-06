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

#include "SingleFrameViewerApplication.h"

#include "../../../../OrthancStone/Sources/Platforms/WebAssembly/WebAssemblyCairoViewport.h"
#include "../../../../OrthancStone/Sources/Platforms/WebAssembly/WebGLViewport.h"
#include "../../../../OrthancStone/Sources/Platforms/WebAssembly/WebGLViewportsRegistry.h"
#include "../../../../OrthancStone/Sources/StoneApplication.h"
#include "../../../../OrthancStone/Sources/StoneException.h"
#include "../../../../OrthancStone/Sources/StoneInitialization.h"

#include <Compatibility.h>  // For std::unique_ptr<>
#include <Toolbox.h>

#include <EmbeddedResources.h>  // For Orthanc::EmbeddedResources::UBUNTU_FONT


#include <emscripten.h>
#include <emscripten/html5.h>


#define DISPATCH_JAVASCRIPT_EVENT(name)                         \
  EM_ASM(                                                       \
    const customEvent = document.createEvent("CustomEvent");    \
    customEvent.initCustomEvent(name, false, false, undefined); \
    window.dispatchEvent(customEvent);                          \
    );

#define EXTERN_CATCH_EXCEPTIONS                         \
  catch (Orthanc::OrthancException& e)                  \
  {                                                     \
    LOG(ERROR) << "OrthancException: " << e.What();     \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (OrthancStone::StoneException& e)               \
  {                                                     \
    LOG(ERROR) << "StoneException: " << e.What();       \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (std::exception& e)                             \
  {                                                     \
    LOG(ERROR) << "Runtime error: " << e.what();        \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }                                                     \
  catch (...)                                           \
  {                                                     \
    LOG(ERROR) << "Native exception";                   \
    DISPATCH_JAVASCRIPT_EVENT("StoneException");        \
  }



namespace OrthancStone
{
  class SingleViewportWebAssemblyApplication : public StoneApplication
  {
  private:
    boost::shared_ptr<ISingleViewportApplicationCore>  core_;
    boost::shared_ptr<WebAssemblyViewport>             viewport_;

    static EM_BOOL OnKeyDown(int eventType,
                             const EmscriptenKeyboardEvent *e,
                             void *userData)
    {
      SingleViewportWebAssemblyApplication& that = *reinterpret_cast<SingleViewportWebAssemblyApplication*>(userData);

      if (e != NULL &&
          e->key[0] != '\0' &&
          e->key[1] == '\0' &&
          e->key[0] >= 32 &&
          e->key[1] <= 126)
      {
        const char key = static_cast<char>(e->key[0]);

        std::unique_ptr<IEnvironment::ILock> environmentLock(that.GetContext()->GetEnvironment().AcquireLock());
        that.core_->HandleKeyDown(*environmentLock, key);
      }

      return EM_FALSE; // Don't prevent the browser's default behavior
    }

  public:
    SingleViewportWebAssemblyApplication(const Configuration& configuration,
                                         const boost::shared_ptr<ISingleViewportApplicationCore>& core) :
      StoneApplication(configuration),
      core_(core)
    {
      if (!core)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }

      Start();
    }

    void Setup(const std::string& canvasId,
               bool useOpenGL)
    {
      if (viewport_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        if (useOpenGL)
        {
          viewport_ = OrthancStone::WebGLViewport::Create(canvasId);
        }
        else
        {
          viewport_ = OrthancStone::WebAssemblyCairoViewport::Create(canvasId);
        }

        std::string font;
        Orthanc::EmbeddedResources::GetFileResource(font, Orthanc::EmbeddedResources::UBUNTU_FONT);

        {
          std::unique_ptr<IViewport::ILock> viewportLock(viewport_->Lock());
          viewportLock->GetCompositor().SetFont(0, font, 16, Orthanc::Encoding_Latin1);
        }

        core_->CreateComponents(GetContext(), viewport_);

        viewport_->AcquireInteractor(core_->CreateMouseInteractor());

        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EM_FALSE, OnKeyDown);
      }
    }

    const boost::shared_ptr<ISingleViewportApplicationCore>& GetCore() const
    {
      return core_;
    }
  };
}


static boost::shared_ptr<OrthancStone::SingleViewportWebAssemblyApplication> application2_;
static boost::shared_ptr<OrthancStone::SingleFrameViewerApplication>  application_;

extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    try
    {
      OrthancStone::StoneInitialize();

      Orthanc::Logging::EnableInfoLevel(true);
      //Orthanc::Logging::EnableTraceLevel(true);
      LOG(WARNING) << "Initializing native Stone";

      LOG(WARNING) << "Compiled with Emscripten " << __EMSCRIPTEN_major__
                   << "." << __EMSCRIPTEN_minor__
                   << "." << __EMSCRIPTEN_tiny__;

      LOG(INFO) << "Endianness: " << Orthanc::EnumerationToString(Orthanc::Toolbox::DetectEndianness());

      OrthancStone::StoneApplication::Configuration configuration;
      configuration.SetLocalOrthancRoot("..");
      configuration.SetDicomCacheSize(128 * 1024 * 1024);  // 128MB

      boost::shared_ptr<OrthancStone::SimpleViewerCore> core(new OrthancStone::SimpleViewerCore);
      application2_.reset(new OrthancStone::SingleViewportWebAssemblyApplication(configuration, core));

#if 0
      OrthancStone::StoneApplication::Initialize(configuration);
#endif
  
      DISPATCH_JAVASCRIPT_EVENT("WasmModuleInitialized");
    }
    EXTERN_CATCH_EXCEPTIONS;

    return 0;
  }
  
  EMSCRIPTEN_KEEPALIVE
  void InitializeViewport(const char* canvasId)
  {
    try
    {
      application2_->Setup(canvasId, true);

#if 0
      if (application_.get() != NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                        "Only one single viewport is available for this application");
      }

      boost::shared_ptr<OrthancStone::WebGLViewport> viewport(
        OrthancStone::WebGLViewportsRegistry::GetGlobalRegistry().Add(canvasId));
      application_ = OrthancStone::SingleFrameViewerApplication::Create(OrthancStone::StoneApplication::GetInstance(), viewport);

      {
        OrthancStone::WebGLViewportsRegistry::Accessor accessor(
          OrthancStone::WebGLViewportsRegistry::GetGlobalRegistry(), canvasId);

        if (accessor.IsValid())
        {
          accessor.GetViewport().Invalidate();
        }
      }
#endif
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  
  EMSCRIPTEN_KEEPALIVE
  void LoadFromOrthanc(const char* instance,
                       int frame)
  {
    try
    {
      dynamic_cast<OrthancStone::SimpleViewerCore&>(*application2_->GetCore()).LoadFrameFromOrthanc(instance, frame);

#if 0
      if (application_.get() != NULL)
      {
        OrthancStone::DicomSource source;
        application_->LoadOrthancFrame(source, instance, frame);
      }
#endif
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  
  EMSCRIPTEN_KEEPALIVE
  void LoadFromDicomWeb(const char* server,
                        const char* studyInstanceUid,
                        const char* seriesInstanceUid,
                        const char* sopInstanceUid,
                        int frame)
  {
    try
    {
      dynamic_cast<OrthancStone::SimpleViewerCore&>(*application2_->GetCore()).LoadFrameFromDicomWeb(
        server, studyInstanceUid, seriesInstanceUid, sopInstanceUid, frame);

#if 0
      if (application_.get() != NULL)
      {
        OrthancStone::DicomSource source;
        source.SetDicomWebThroughOrthancSource(server);
        application_->LoadDicomWebFrame(source, studyInstanceUid, seriesInstanceUid,
                                        sopInstanceUid, frame);
      }
#endif
    }
    EXTERN_CATCH_EXCEPTIONS;
  }
}
