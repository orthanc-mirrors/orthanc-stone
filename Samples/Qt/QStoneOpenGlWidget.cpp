#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include "QStoneOpenGlWidget.h"

void QStoneOpenGlWidget::initializeGL()
{
  glewInit();
}

void QStoneOpenGlWidget::MakeCurrent()
{
  this->makeCurrent();
}

void QStoneOpenGlWidget::resizeGL(int w, int h)
{

}

void QStoneOpenGlWidget::paintGL()
{
  if (compositor_)
  {
    compositor_->Refresh();
  }
  doneCurrent();
}

