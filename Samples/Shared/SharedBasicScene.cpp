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

#include "SharedBasicScene.h"

// From Stone
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/PolylineSceneLayer.h"
#include "../../Framework/Scene2D/TextSceneLayer.h"

#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"

#include "../../Framework/Scene2D/CairoCompositor.h"

// From Orthanc framework
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>

using namespace OrthancStone;

const unsigned int BASIC_SCENE_FONT_SIZE = 32;
const int BASIC_SCENE_LAYER_POSITION = 150;

void PrepareScene(Scene2D& scene)
{
  //Scene2D& scene(*controller->GetScene());
  // Texture of 2x2 size
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
    l->SetAngle(20.0 / 180.0 * 3.14);
    scene.SetLayer(14, l.release());
  }

  // Texture of 1x1 size
  {
    Orthanc::Image i(Orthanc::PixelFormat_RGB24, 1, 1, false);

    uint8_t *p = reinterpret_cast<uint8_t*>(i.GetRow(0));
    p[0] = 255;
    p[1] = 0;
    p[2] = 0;

    std::auto_ptr<ColorTextureSceneLayer> l(new ColorTextureSceneLayer(i));
    l->SetOrigin(-2, 1);
    l->SetAngle(20.0 / 180.0 * 3.14);
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

    //    layer->SetColor(0,255, 255);
    scene.SetLayer(50, layer.release());
  }

  // Some text
  {
    std::auto_ptr<TextSceneLayer> layer(new TextSceneLayer);
    layer->SetText("Hello");
    scene.SetLayer(100, layer.release());
  }
}

#if ORTHANC_SANDBOXED == 0
void TakeScreenshot(const std::string& target,
                    const OrthancStone::Scene2D& scene,
                    unsigned int canvasWidth,
                    unsigned int canvasHeight)
{
  using namespace OrthancStone;
  // Take a screenshot, then save it as PNG file
  CairoCompositor compositor(scene, canvasWidth, canvasHeight);
  compositor.SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT, BASIC_SCENE_FONT_SIZE, Orthanc::Encoding_Latin1);
  compositor.Refresh();

  Orthanc::ImageAccessor canvas;
  compositor.GetCanvas().GetReadOnlyAccessor(canvas);

  Orthanc::Image png(Orthanc::PixelFormat_RGB24, canvas.GetWidth(), canvas.GetHeight(), false);
  Orthanc::ImageProcessing::Convert(png, canvas);

  Orthanc::PngWriter writer;
  writer.WriteToFile(target, png);
}
#endif

void ShowCursorInfo(Scene2D& scene, const PointerEvent& pointerEvent)
{
  ScenePoint2D p = pointerEvent.GetMainPosition().Apply(scene.GetCanvasToSceneTransform());

  char buf[64];
  sprintf(buf, "(%0.02f,%0.02f)", p.GetX(), p.GetY());

  if (scene.HasLayer(BASIC_SCENE_LAYER_POSITION))
  {
    TextSceneLayer& layer =
        dynamic_cast<TextSceneLayer&>(scene.GetLayer(BASIC_SCENE_LAYER_POSITION));
    layer.SetText(buf);
    layer.SetPosition(p.GetX(), p.GetY());
  }
  else
  {
    std::auto_ptr<TextSceneLayer>
        layer(new TextSceneLayer);
    layer->SetColor(0, 255, 0);
    layer->SetText(buf);
    layer->SetBorder(20);
    layer->SetAnchor(BitmapAnchor_BottomCenter);
    layer->SetPosition(p.GetX(), p.GetY());
    scene.SetLayer(BASIC_SCENE_LAYER_POSITION, layer.release());
  }
}



bool BasicScene2DInteractor::OnMouseEvent(const GuiAdapterMouseEvent& event, const PointerEvent& pointerEvent)
{
  if (currentTracker_.get() != NULL)
  {
    switch (event.type)
    {
    case GUIADAPTER_EVENT_MOUSEUP:
    {
      currentTracker_->PointerUp(pointerEvent);
      if (!currentTracker_->IsAlive())
      {
        currentTracker_.reset();
      }
    };break;
    case GUIADAPTER_EVENT_MOUSEMOVE:
    {
      currentTracker_->PointerMove(pointerEvent);
    };break;
    default:
      return false;
    }
    return true;
  }
  else if (event.type == GUIADAPTER_EVENT_MOUSEDOWN)
  {
    if (event.button == GUIADAPTER_MOUSEBUTTON_LEFT)
    {
      currentTracker_.reset(new RotateSceneTracker(viewportController_, pointerEvent));
    }
    else if (event.button == GUIADAPTER_MOUSEBUTTON_MIDDLE)
    {
      currentTracker_.reset(new PanSceneTracker(viewportController_, pointerEvent));
    }
    else if (event.button == GUIADAPTER_MOUSEBUTTON_RIGHT && compositor_.get() != NULL)
    {
      currentTracker_.reset(new ZoomSceneTracker(viewportController_, pointerEvent, compositor_->GetHeight()));
    }
  }
  else if (event.type == GUIADAPTER_EVENT_MOUSEMOVE)
  {
    if (showCursorInfo_)
    {
      Scene2D& scene(*(viewportController_->GetScene()));
      ShowCursorInfo(scene, pointerEvent);
    }
    return true;
  }
  return false;
}

bool BasicScene2DInteractor::OnKeyboardEvent(const GuiAdapterKeyboardEvent& guiEvent)
{
  if (guiEvent.type == GUIADAPTER_EVENT_KEYDOWN)
  {
    switch (guiEvent.sym[0])
    {
    case 's':
    {
      viewportController_->FitContent(compositor_->GetWidth(), compositor_->GetHeight());
      return true;
    };
#if ORTHANC_SANDBOXED == 0
    case 'c':
    {
      Scene2D& scene(*(viewportController_->GetScene()));
      TakeScreenshot("screenshot.png", scene, compositor_->GetWidth(), compositor_->GetHeight());
      return true;
    }
#endif
    case 'd':
    {
      showCursorInfo_ = !showCursorInfo_;
      if (!showCursorInfo_)
      {
        Scene2D& scene(*(viewportController_->GetScene()));
        scene.DeleteLayer(BASIC_SCENE_LAYER_POSITION);
      }

      return true;
    }
    }
  }
  return false;
}

bool BasicScene2DInteractor::OnWheelEvent(const GuiAdapterWheelEvent& guiEvent)
{
  return false;
}
