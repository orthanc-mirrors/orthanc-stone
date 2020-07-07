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

#include "QCairoWidget.h"

#include <QPainter>
#include <QPaintEvent>

#include <stdexcept>


QCairoWidget::StoneObserver::StoneObserver(QCairoWidget& that,
                                           Deprecated::IViewport& viewport,
                                           OrthancStone::MessageBroker& broker) :
  OrthancStone::IObserver(broker),
  that_(that)
{
  // get notified each time the content of the central viewport changes
  viewport.RegisterObserverCallback(
    new OrthancStone::Callable<StoneObserver, Deprecated::IViewport::ViewportChangedMessage>
    (*this, &StoneObserver::OnViewportChanged));
}


QCairoWidget::QCairoWidget(QWidget *parent) :
  QWidget(parent),
  context_(NULL)
{
  setFocusPolicy(Qt::StrongFocus); // catch keyPressEvents
}


void QCairoWidget::SetContext(OrthancStone::NativeStoneApplicationContext& context)
{
  context_ = &context;

  {
    OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);
    observer_.reset(new StoneObserver(*this,
                                      locker.GetCentralViewport(),
                                      locker.GetMessageBroker()));
  }
}


void QCairoWidget::paintEvent(QPaintEvent* /*event*/)
{
  QPainter painter(this);

  if (image_.get() != NULL &&
      context_ != NULL)
  {
    OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);
    Deprecated::IViewport& viewport = locker.GetCentralViewport();
    Orthanc::ImageAccessor a;
    surface_.GetWriteableAccessor(a);
    viewport.Render(a);
    painter.drawImage(0, 0, *image_);
  }
  else
  {
    painter.fillRect(rect(), Qt::red);
  }
}

OrthancStone::KeyboardModifiers GetKeyboardModifiers(QInputEvent* event)
{
  Qt::KeyboardModifiers qtModifiers = event->modifiers();
  int stoneModifiers = static_cast<int>(OrthancStone::KeyboardModifiers_None);
  if ((qtModifiers & Qt::AltModifier) != 0)
  {
    stoneModifiers |= static_cast<int>(OrthancStone::KeyboardModifiers_Alt);
  }
  if ((qtModifiers & Qt::ControlModifier) != 0)
  {
    stoneModifiers |= static_cast<int>(OrthancStone::KeyboardModifiers_Control);
  }
  if ((qtModifiers & Qt::ShiftModifier) != 0)
  {
    stoneModifiers |= static_cast<int>(OrthancStone::KeyboardModifiers_Shift);
  }
  return static_cast<OrthancStone::KeyboardModifiers>(stoneModifiers);
}

void QCairoWidget::mousePressEvent(QMouseEvent* event)
{
  OrthancStone::KeyboardModifiers stoneModifiers = GetKeyboardModifiers(event);

  OrthancStone::MouseButton button;

  switch (event->button())
  {
    case Qt::LeftButton:
      button = OrthancStone::MouseButton_Left;
      break;

    case Qt::RightButton:
      button = OrthancStone::MouseButton_Right;
      break;

    case Qt::MiddleButton:
      button = OrthancStone::MouseButton_Middle;
      break;

    default:
      return;  // Unsupported button
  }

  {
    OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);
    locker.GetCentralViewport().MouseDown(button, event->pos().x(), event->pos().y(), stoneModifiers, std::vector<Deprecated::Touch>());
  }
}


void QCairoWidget::mouseReleaseEvent(QMouseEvent* /*eventNotUsed*/)
{
  OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);
  locker.GetCentralViewport().MouseLeave();
}


void QCairoWidget::mouseMoveEvent(QMouseEvent* event)
{
  OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);
  locker.GetCentralViewport().MouseMove(event->pos().x(), event->pos().y(), std::vector<Deprecated::Touch>());
}


void QCairoWidget::wheelEvent(QWheelEvent * event)
{
  OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);

  OrthancStone::KeyboardModifiers stoneModifiers = GetKeyboardModifiers(event);

  if (event->orientation() == Qt::Vertical)
  {
    if (event->delta() < 0)  // TODO: compare direction with SDL and make sure we send the same directions
    {
       locker.GetCentralViewport().MouseWheel(OrthancStone::MouseWheelDirection_Up, event->pos().x(), event->pos().y(), stoneModifiers);
    }
    else
    {
      locker.GetCentralViewport().MouseWheel(OrthancStone::MouseWheelDirection_Down, event->pos().x(), event->pos().y(), stoneModifiers);
    }
  }
}

void QCairoWidget::keyPressEvent(QKeyEvent *event)
{
  using namespace OrthancStone;

  OrthancStone::KeyboardModifiers stoneModifiers = GetKeyboardModifiers(event);

  OrthancStone::KeyboardKeys keyType = OrthancStone::KeyboardKeys_Generic;
  char keyChar = event->text()[0].toLatin1();

#define CASE_QT_KEY_TO_ORTHANC(qt, o) case qt: keyType = o; break;
  if (keyChar == 0)
  {
    switch (event->key())
    {
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_Up, KeyboardKeys_Up);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_Down, KeyboardKeys_Down);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_Left, KeyboardKeys_Left);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_Right, KeyboardKeys_Right);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F1, KeyboardKeys_F1);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F2, KeyboardKeys_F2);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F3, KeyboardKeys_F3);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F4, KeyboardKeys_F4);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F5, KeyboardKeys_F5);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F6, KeyboardKeys_F6);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F7, KeyboardKeys_F7);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F8, KeyboardKeys_F8);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F9, KeyboardKeys_F9);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F10, KeyboardKeys_F10);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F11, KeyboardKeys_F11);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_F12, KeyboardKeys_F12);
    default:
      break;
    }
  }
  else if (keyChar == 127)
  {
    switch (event->key())
    {
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_Delete, KeyboardKeys_Delete);
      CASE_QT_KEY_TO_ORTHANC(Qt::Key_Backspace, KeyboardKeys_Backspace);
    default:
      break;
    }
  }

  {
    OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);    
    locker.GetCentralViewport().KeyPressed(keyType, keyChar, stoneModifiers);
  }
}


void QCairoWidget::resizeEvent(QResizeEvent* event)
{
  grabGesture(Qt::PanGesture);
  QWidget::resizeEvent(event);

  if (event)
  {
    surface_.SetSize(event->size().width(), event->size().height(), true);

    image_.reset(new QImage(reinterpret_cast<uchar*>(surface_.GetBuffer()),
                            event->size().width(), 
                            event->size().height(),
                            surface_.GetPitch(),
                            QImage::Format_RGB32));

    {
      OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker locker(*context_);
      locker.GetCentralViewport().SetSize(event->size().width(), event->size().height());
    }
  }
}
