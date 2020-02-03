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

#define GLEW_STATIC 1
// From Stone
#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include "../../Applications/Sdl/SdlWindow.h"
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

#include <stdio.h>
#include <QDebug>
#include <QWindow>

#include "BasicScene.h"


using namespace OrthancStone;



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

int main(int argc, char* argv[])
{
  QApplication a(argc, argv);

  OrthancStone::Samples::BasicSceneWindow window;
  window.show();
  window.GetOpenGlWidget().Init();

  MessageBroker broker;
  boost::shared_ptr<UndoStack> undoStack(new UndoStack);
  boost::shared_ptr<ViewportController> controller = boost::make_shared<ViewportController>(undoStack, boost::ref(broker), window.GetOpenGlWidget());
  PrepareScene(controller->GetScene());

  window.GetOpenGlWidget().GetCompositor().SetFont(0, Orthanc::EmbeddedResources::UBUNTU_FONT,
                     BASIC_SCENE_FONT_SIZE, Orthanc::Encoding_Latin1);

  boost::shared_ptr<OrthancStone::Scene2DInteractor> interactor(new BasicScene2DInteractor(controller));
  window.GetOpenGlWidget().SetInteractor(interactor);

  controller->FitContent();

  return a.exec();
}
