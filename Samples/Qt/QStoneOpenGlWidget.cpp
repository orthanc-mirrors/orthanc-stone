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
    const IViewport& viewport)
{
  guiEvent.targetX = qtEvent.x();
  guiEvent.targetY = qtEvent.y();
  pointerEvent.AddPosition(viewport.GetPixelCenterCoordinates(guiEvent.targetX, guiEvent.targetY));

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
  ConvertFromPlatform(guiEvent, pointerEvent, *qtEvent, *this);
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
  mouseEvent(qtEvent, GUIADAPTER_EVENT_MOUSEMOVE);
}

void QStoneOpenGlWidget::mouseReleaseEvent(QMouseEvent* qtEvent)
{
  mouseEvent(qtEvent, GUIADAPTER_EVENT_MOUSEUP);
}

void ConvertFromPlatform(
    OrthancStone::GuiAdapterKeyboardEvent& guiEvent,
    const QKeyEvent& qtEvent)
{
  if (qtEvent.text().length() > 0)
  {
    guiEvent.sym[0] = qtEvent.text()[0].cell();
  }
  else
  {
    guiEvent.sym[0] = 0;
  }
  guiEvent.sym[1] = 0;

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


bool QStoneOpenGlWidget::keyEvent(QKeyEvent* qtEvent, OrthancStone::GuiAdapterHidEventType guiEventType)
{
  bool handled = false;
  OrthancStone::GuiAdapterKeyboardEvent guiEvent;
  ConvertFromPlatform(guiEvent, *qtEvent);
  guiEvent.type = guiEventType;

  if (sceneInteractor_.get() != NULL && compositor_.get() != NULL)
  {
    handled = sceneInteractor_->OnKeyboardEvent(guiEvent);

    if (handled)
    {
      // force redraw of the OpenGL widget
      update();
    }
  }
  return handled;
}

void QStoneOpenGlWidget::keyPressEvent(QKeyEvent *qtEvent)
{
  bool handled = keyEvent(qtEvent, GUIADAPTER_EVENT_KEYDOWN);
  if (!handled)
  {
    QOpenGLWidget::keyPressEvent(qtEvent);
  }
}

void QStoneOpenGlWidget::keyReleaseEvent(QKeyEvent *qtEvent)
{
  bool handled = keyEvent(qtEvent, GUIADAPTER_EVENT_KEYUP);
  if (!handled)
  {
    QOpenGLWidget::keyPressEvent(qtEvent);
  }
}

void QStoneOpenGlWidget::wheelEvent(QWheelEvent *qtEvent)
{
  OrthancStone::GuiAdapterWheelEvent guiEvent;
  throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);

  // force redraw of the OpenGL widget
  update();
}
