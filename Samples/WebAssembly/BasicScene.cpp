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


#include "dev.h"

#include <emscripten.h>
#include <emscripten/html5.h>

// From Stone
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/StoneInitialization.h"

// From Orthanc framework
#include <Core/Images/Image.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include "../Shared/SharedBasicScene.h"

std::auto_ptr<OrthancStone::WebAssemblyViewport>  viewport1_;
std::auto_ptr<OrthancStone::WebAssemblyViewport>  viewport2_;
std::auto_ptr<OrthancStone::WebAssemblyViewport>  viewport3_;
OrthancStone::MessageBroker  broker_;

EM_BOOL OnWindowResize(
  int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
  if (viewport1_.get() != NULL)
  {
    viewport1_->UpdateSize();
  }
  
  if (viewport2_.get() != NULL)
  {
    viewport2_->UpdateSize();
  }
  
  if (viewport3_.get() != NULL)
  {
    viewport3_->UpdateSize();
  }
  
  return true;
}

extern "C"
{
  int main(int argc, char const *argv[]) 
  {
    OrthancStone::StoneInitialize();
    // Orthanc::Logging::EnableInfoLevel(true);
    // Orthanc::Logging::EnableTraceLevel(true);
    EM_ASM(window.dispatchEvent(new CustomEvent("WebAssemblyLoaded")););
  }

  EMSCRIPTEN_KEEPALIVE
  void Initialize()
  {
    viewport1_.reset(
      new OrthancStone::WebAssemblyViewport(broker_, "mycanvas1"));
    PrepareScene(viewport1_->GetScene());
    viewport1_->UpdateSize();

    viewport2_.reset(
      new OrthancStone::WebAssemblyViewport(broker_, "mycanvas2"));
    PrepareScene(viewport2_->GetScene());
    viewport2_->UpdateSize();

    viewport3_.reset(
      new OrthancStone::WebAssemblyViewport(broker_, "mycanvas3"));
    PrepareScene(viewport3_->GetScene());
    viewport3_->UpdateSize();

    emscripten_set_resize_callback("#window", NULL, false, OnWindowResize);
  }
}
