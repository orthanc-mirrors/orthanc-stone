#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include "QStoneOpenGlWidget.h"

void QStoneOpenGlWidget::initializeGL()
{
  // Set up the rendering context, load shaders and other resources, etc.:
  QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
  f->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void QStoneOpenGlWidget::resizeGL(int w, int h)
{

}

void QStoneOpenGlWidget::paintGL()
{
  makeCurrent();

  //        // Draw the scene:
  //        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
  //        f->glClear(GL_COLOR_BUFFER_BIT);
  //        f->glClearColor(1.0f, 0.3f, 0.5f, 1.0f);

  if (compositor_)
  {
    compositor_->Refresh();
  }
  doneCurrent();
}

