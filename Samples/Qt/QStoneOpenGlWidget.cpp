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
    OrthancStone::GuiAdapterMouseEvent& guiEvent,
    PointerEvent& pointerEvent,
    const QMouseEvent& qtEvent,
    const OrthancStone::OpenGLCompositor& compositor)
{
  guiEvent.targetX = qtEvent.x();
  guiEvent.targetY = qtEvent.y();
  pointerEvent.AddPosition(compositor.GetPixelCenterCoordinates(guiEvent.targetX, guiEvent.targetY));

  switch (qtEvent.button())
  {
  case Qt::LeftButton: guiEvent.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_LEFT; break;
  case Qt::MiddleButton: guiEvent.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_MIDDLE; break;
  case Qt::RightButton: guiEvent.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_RIGHT; break;
  default:
    guiEvent.button = OrthancStone::GUIADAPTER_MOUSEBUTTON_LEFT;
  }

  if (qtEvent.modifiers().testFlag(Qt::ShiftModifier))
  {
    guiEvent.shiftKey = true;
  }
  if (qtEvent.modifiers().testFlag(Qt::ControlModifier))
  {
    guiEvent.ctrlKey = true;
  }
  if (qtEvent.modifiers().testFlag(Qt::AltModifier))
  {
    guiEvent.altKey = true;
  }

}

void QStoneOpenGlWidget::mouseEvent(QMouseEvent* qtEvent, OrthancStone::GuiAdapterHidEventType guiEventType)
{
  OrthancStone::GuiAdapterMouseEvent guiEvent;
  PointerEvent pointerEvent;
  ConvertFromPlatform(guiEvent, pointerEvent, *qtEvent, *compositor_);
  guiEvent.type = guiEventType;

  if (sceneInteractor_.get() != NULL && compositor_.get() != NULL)
  {
    sceneInteractor_->OnMouseEvent(guiEvent, pointerEvent);
  }

  // force redraw of the OpenGL widget
  update();
}

void QStoneOpenGlWidget::mousePressEvent(QMouseEvent* qtEvent)
{
  mouseEvent(qtEvent, GUIADAPTER_EVENT_MOUSEDOWN);
}

void QStoneOpenGlWidget::mouseMoveEvent(QMouseEvent* qtEvent)
{
  mouseEvent(qtEvent, GUIADAPTER_EVENT_MOUSEDOWN);
}

void QStoneOpenGlWidget::mouseReleaseEvent(QMouseEvent* qtEvent)
{
  mouseEvent(qtEvent, GUIADAPTER_EVENT_MOUSEUP);
}
