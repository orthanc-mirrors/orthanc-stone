#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include "QStoneOpenGlWidget.h"

#include <QMouseEvent>

using namespace OrthancStone;

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

void ConvertFromPlatform(
  OrthancStone::GuiAdapterMouseEvent& dest,
  const QMouseEvent& qtEvent)
{
  dest.targetX = qtEvent.x();
  dest.targetY = qtEvent.y();

  switch (qtEvent.button())
  {
    case Qt::LeftButton: dest.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_LEFT; break;
    case Qt::MiddleButton: dest.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_MIDDLE; break;
    case Qt::RightButton: dest.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_RIGHT; break;
  default:
    dest.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_LEFT;
  }

  if (qtEvent.modifiers().testFlag(Qt::ShiftModifier))
  {
    dest.shiftKey = true;
  }
  if (qtEvent.modifiers().testFlag(Qt::ControlModifier))
  {
    dest.ctrlKey = true;
  }
  if (qtEvent.modifiers().testFlag(Qt::AltModifier))
  {
    dest.altKey = true;
  }

}



void QStoneOpenGlWidget::mousePressEvent(QMouseEvent* qtEvent)
{
  OrthancStone::GuiAdapterMouseEvent event;
  ConvertFromPlatform(event, *qtEvent);

  if (sceneInteractor_.get() != NULL)
  {
    sceneInteractor_->OnMouseEvent(event);
  }


  // convert
//TODO  event->

//  sceneInteractor_->OnMouseEvent(event);
}

