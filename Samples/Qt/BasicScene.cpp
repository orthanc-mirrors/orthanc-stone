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

#define GLEW_STATIC 1
// From Stone
#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include "../../Applications/Sdl/SdlOpenGLWindow.h"
#include "../../Framework/Scene2D/CairoCompositor.h"
#include "../../Framework/Scene2D/ColorTextureSceneLayer.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Scene2D/PanSceneTracker.h"
#include "../../Framework/Scene2D/RotateSceneTracker.h"
#include "../../Framework/Scene2D/Scene2D.h"
#include "../../Framework/Scene2D/ZoomSceneTracker.h"
#include "../../Framework/Scene2DViewport/ViewportController.h"
#include "../../Framework/Scene2DViewport/UndoStack.h"

#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Messages/MessageBroker.h"

// From Orthanc framework
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>

#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include "EmbeddedResources.h"

//#include <SDL.h>
#include <stdio.h>
#include <QDebug>
#include <QWindow>

static const unsigned int FONT_SIZE = 32;
static const int LAYER_POSITION = 150;

using namespace OrthancStone;

void PrepareScene(boost::shared_ptr<OrthancStone::ViewportController> controller)
{
  Scene2D& scene(*controller->GetScene());
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


static void GLAPIENTRY OpenGLMessageCallback(GLenum source,
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

extern void InitGL();

#include <QApplication>
#include "BasicSceneWindow.h"
#include "Scene2DInteractor.h"

int main(int argc, char* argv[])
{
  {
    QApplication a(argc, argv);

    QSurfaceFormat requestedFormat;
    requestedFormat.setVersion( 2, 0 );

    OrthancStone::Samples::BasicSceneWindow window;
    window.show();

    MessageBroker broker;
    boost::shared_ptr<UndoStack> undoStack(new UndoStack);
    boost::shared_ptr<ViewportController> controller = boost::make_shared<ViewportController>(
      undoStack, boost::ref(broker));
    PrepareScene(controller);

    boost::shared_ptr<OrthancStone::Scene2DInteractor> interactor(new BasicScene2DInteractor(controller));
    window.GetOpenGlWidget().SetInteractor(interactor);

    QOpenGLContext * context = new QOpenGLContext;
    context->setFormat( requestedFormat );
    context->create();
    context->makeCurrent(window.GetOpenGlWidget().context()->surface());

    boost::shared_ptr<OpenGLCompositor> compositor = boost::make_shared<OpenGLCompositor>(window.GetOpenGlWidget(), *controller->GetScene());
    compositor->SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
                       FONT_SIZE, Orthanc::Encoding_Latin1);

    interactor->SetCompositor(compositor);
    window.GetOpenGlWidget().SetCompositor(compositor);

    return a.exec();
  }
}
