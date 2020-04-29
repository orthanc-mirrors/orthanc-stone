/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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

#include "RtViewerApp.h"
#include "RtViewerView.h"
#include "SampleHelpers.h"

// Stone of Orthanc includes
#include <Framework/Loaders/WebAssemblyLoadersContext.h>
//#include <Framework/OpenGL/WebAssemblyOpenGLContext.h>
#include <Framework/Viewport/WebGLViewport.h>
#include <Framework/StoneException.h>
#include <Framework/StoneInitialization.h>

#include <Framework/Loaders/WebAssemblyLoadersContext.h>

#include <Framework/StoneException.h>
#include <Framework/StoneInitialization.h>

#include <Core/Toolbox.h>

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
// #include <boost/pointer_cast.hpp> this include might be necessary in more recent boost versions

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
  boost::shared_ptr<IViewport> RtViewerView::CreateViewport(
    const std::string& canvasId)
  {
    return WebGLViewport::Create(canvasId);
  }

  void RtViewerView::TakeScreenshot(const std::string& target,
                                   unsigned int canvasWidth,
                                   unsigned int canvasHeight)
  {
    throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
  }


  void RtViewerApp::RunWasm()
  {
    loadersContext_.reset(new OrthancStone::WebAssemblyLoadersContext(1, 4, 1));

    // we are in WASM --> downcast to concrete type
    boost::shared_ptr<WebAssemblyLoadersContext> loadersContext = 
      boost::dynamic_pointer_cast<WebAssemblyLoadersContext>(loadersContext_);

    if (HasArgument("orthanc"))
      loadersContext->SetLocalOrthanc(GetArgument("orthanc"));
    else 
      loadersContext->SetLocalOrthanc("..");

    loadersContext->SetDicomCacheSize(128 * 1024 * 1024);  // 128MB

    CreateLoaders();
    
    CreateView("RtViewer_Axial", VolumeProjection_Axial);
    CreateView("RtViewer_Coronal", VolumeProjection_Coronal);
    CreateView("RtViewer_Sagittal", VolumeProjection_Sagittal);

    for (size_t i = 0; i < views_.size(); ++i)
    {
      views_[i]->PrepareViewport();
    }

    StartLoaders();

    DefaultViewportInteractor interactor;
  }
}

extern "C"
{
  boost::shared_ptr<OrthancStone::RtViewerApp> g_app;

  int main(int argc, char const *argv[]) 
  {
    try
    {
      OrthancStone::StoneInitialize();
      Orthanc::Logging::Initialize();
      Orthanc::Logging::EnableTraceLevel(true);

      LOG(WARNING) << "Initializing native Stone";

      LOG(WARNING) << "Compiled with Emscripten " << __EMSCRIPTEN_major__
                   << "." << __EMSCRIPTEN_minor__
                   << "." << __EMSCRIPTEN_tiny__;

      LOG(INFO) << "Endianness: " << Orthanc::EnumerationToString(Orthanc::Toolbox::DetectEndianness());

      g_app = OrthancStone::RtViewerApp::Create();
  
      DISPATCH_JAVASCRIPT_EVENT("WasmModuleInitialized");
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  EMSCRIPTEN_KEEPALIVE
  void Initialize(const char* canvasId)
  {
    try
    {
      g_app->RunWasm();
    }
    EXTERN_CATCH_EXCEPTIONS;
  }

  EMSCRIPTEN_KEEPALIVE
  void SetArgument(const char* key, const char* value)
  {
    // This is called for each GET argument (cf. "app.js")
    LOG(INFO) << "Received GET argument: [" << key << "] = [" << value << "]";
    g_app->SetArgument(key, value);
  }

}
