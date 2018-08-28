/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

QCairoWidget::QCairoWidget(QWidget *parent) :
  QWidget(parent),
  context_(NULL)
{
  setFocusPolicy(Qt::StrongFocus); // catch keyPressEvents
}

QCairoWidget::~QCairoWidget()
{
}

void QCairoWidget::SetContext(OrthancStone::BasicNativeApplicationContext& context)
{
  context_ = &context;
  context_->GetCentralViewport().Register(*this); // get notified each time the content of the central viewport changes
}

void QCairoWidget::paintEvent(QPaintEvent* /*event*/)
{
  QPainter painter(this);

  if (image_.get() != NULL && context_ != NULL)
  {
    OrthancStone::BasicNativeApplicationContext::GlobalMutexLocker locker(*context_);
    OrthancStone::IViewport& viewport = context_->GetCentralViewport();
    Orthanc::ImageAccessor a = surface_.GetAccessor();
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
  context_->GetCentralViewport().MouseDown(button, event->pos().x(), event->pos().y(), stoneModifiers);
}


void QCairoWidget::mouseReleaseEvent(QMouseEvent* /*eventNotUsed*/)
{
  context_->GetCentralViewport().MouseLeave();
}


void QCairoWidget::mouseMoveEvent(QMouseEvent* event)
{
  context_->GetCentralViewport().MouseMove(event->pos().x(), event->pos().y());
}


void QCairoWidget::wheelEvent(QWheelEvent * event)
{
  OrthancStone::KeyboardModifiers stoneModifiers = GetKeyboardModifiers(event);

  if (event->orientation() == Qt::Vertical)
  {
    if (event->delta() < 0)  // TODO: compare direction with SDL and make sure we send the same directions
    {
       context_->GetCentralViewport().MouseWheel(OrthancStone::MouseWheelDirection_Up, event->pos().x(), event->pos().y(), stoneModifiers);
    }
    else
    {
      context_->GetCentralViewport().MouseWheel(OrthancStone::MouseWheelDirection_Down, event->pos().x(), event->pos().y(), stoneModifiers);
    }
  }
}

void QCairoWidget::keyPressEvent(QKeyEvent *event)
{
  OrthancStone::KeyboardModifiers stoneModifiers = GetKeyboardModifiers(event);

  context_->GetCentralViewport().KeyPressed(event->text()[0].toLatin1(), stoneModifiers);
}


void QCairoWidget::resizeEvent(QResizeEvent* event)
{
  grabGesture(Qt::PanGesture);
  QWidget::resizeEvent(event);

  if (event)
  {
    surface_.SetSize(event->size().width(), event->size().height());

    image_.reset(new QImage(reinterpret_cast<uchar*>(surface_.GetBuffer()),
                            event->size().width(), 
                            event->size().height(),
                            surface_.GetPitch(),
                            QImage::Format_RGB32));

    context_->GetCentralViewport().SetSize(event->size().width(), event->size().height());

  }
}
