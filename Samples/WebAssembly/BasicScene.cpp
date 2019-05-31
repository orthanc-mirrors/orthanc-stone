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

void PrepareScene(OrthancStone::Scene2D& scene)
{
  using namespace OrthancStone;

  // Texture of 2x2 size
  if (1)
  {
    Orthanc::Image i(Orthanc::PixelFormat_RGB24, 2, 2, false);
    
    uint8_t *p = reinterpret_cast<uint8_t*>(i.GetRow(0));
    p[0] = 255;
    p[1] = 0;
    p[2] = 0;

    p[3] = 0;
    p[4] = 255;
    p[5] = 0;

    p = reinterpret_cast<uint8_t*>(i.GetRow(1));
    p[0] = 0;
    p[1] = 0;
    p[2] = 255;

    p[3] = 255;
    p[4] = 0;
    p[5] = 0;

    scene.SetLayer(12, new ColorTextureSceneLayer(i));

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-3, 2);
    l->SetPixelSpacing(1.5, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene.SetLayer(14, l.release());
  }

  // Texture of 1x1 size
  if (1)
  {
    Orthanc::Image i(Orthanc::PixelFormat_RGB24, 1, 1, false);
    
    uint8_t *p = reinterpret_cast<uint8_t*>(i.GetRow(0));
    p[0] = 255;
    p[1] = 0;
    p[2] = 0;

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-2, 1);
    l->SetAngle(20.0 / 180.0 * M_PI);
    scene.SetLayer(13, l.release());
  }

  // Some lines
  {
    std::auto_ptr<PolylineSceneLayer> layer(new PolylineSceneLayer);

    layer->SetThickness(1);

    PolylineSceneLayer::Chain chain;
    chain.push_back(ScenePoint2D(0 - 0.5, 0 - 0.5));
    chain.push_back(ScenePoint2D(0 - 0.5, 2 - 0.5));
    chain.push_back(ScenePoint2D(2 - 0.5, 2 - 0.5));
    chain.push_back(ScenePoint2D(2 - 0.5, 0 - 0.5));
    layer->AddChain(chain, true, 255, 0, 0);

    chain.clear();
    chain.push_back(ScenePoint2D(-5, -5));
    chain.push_back(ScenePoint2D(5, -5));
    chain.push_back(ScenePoint2D(5, 5));
    chain.push_back(ScenePoint2D(-5, 5));
    layer->AddChain(chain, true, 0, 255, 0);

    double dy = 1.01;
    chain.clear();
    chain.push_back(ScenePoint2D(-4, -4));
    chain.push_back(ScenePoint2D(4, -4 + dy));
    chain.push_back(ScenePoint2D(-4, -4 + 2.0 * dy));
    chain.push_back(ScenePoint2D(4, 2));
    layer->AddChain(chain, false, 0, 0, 255);

    scene.SetLayer(50, layer.release());
  }

  // Some text
  if (1)
  {
    std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
    layer->SetText("Hello");
    scene.SetLayer(100, layer.release());
  }
}


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
