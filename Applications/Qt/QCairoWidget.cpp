#include "QCairoWidget.h"

#include <QPainter>
#include <QPaintEvent>

#include <stdexcept>

QCairoWidget::QCairoWidget(QWidget *parent) :
  QWidget(parent),
  context_(NULL),
  isMouseOver_(false),
  mouseOverX_(0),
  mouseOverY_(0)
{
  setMouseTracking(true);
  updateNeeded_ = true;
}



QCairoWidget::~QCairoWidget()
{
}


void QCairoWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);

  if (image_.get() != NULL && context_ != NULL)
  {
    OrthancStone::BasicNativeApplicationContext::GlobalMutexLocker locker(*context_);
    OrthancStone::IViewport& vp = context_->GetCentralViewport();
    Orthanc::ImageAccessor a = surface_.GetAccessor();
    vp.Render(a);
    //image_->fill(0);
    painter.drawImage(0, 0, *image_);
  }
  else
  {
    painter.fillRect(rect(), Qt::red);
  }
}


//static void GetPixelCenter(double& x,
//                           double& y,
//                           const QMouseEvent* event)
//{
//  x = static_cast<double>(event->x()) + 0.5;
//  y = static_cast<double>(event->y()) + 0.5;
//}


//void QCairoWidget::mousePressEvent(QMouseEvent* event)
//{
//  if (mouseTracker_.get() == NULL)
//  {
//    OrthancStone::MouseButton button;

//    switch (event->button())
//    {
//      case Qt::LeftButton:
//        button = OrthancStone::MouseButton_Left;
//        break;

//      case Qt::RightButton:
//        button = OrthancStone::MouseButton_Right;
//        break;

//      case Qt::MiddleButton:
//        button = OrthancStone::MouseButton_Middle;
//        break;

//      default:
//        return;  // Unsupported button
//    }

//    double x, y;
//    GetPixelCenter(x, y, event);
////TODO    mouseTracker_.reset(viewport_.CreateMouseTracker(*scene_, button, x, y));
//    repaint();
//  }
//}


//void QCairoWidget::mouseReleaseEvent(QMouseEvent* event)
//{
//  if (mouseTracker_.get() != NULL)
//  {
//    mouseTracker_->MouseUp();
//    mouseTracker_.reset(NULL);
//    repaint();
//  }
//}


//void QCairoWidget::mouseMoveEvent(QMouseEvent* event)
//{
//  if (mouseTracker_.get() == NULL)
//  {
//    if (rect().contains(event->pos()))
//    {
//      // Mouse over widget
//      isMouseOver_ = true;
//      mouseOverX_ = event->x();
//      mouseOverY_ = event->y();
//    }
//    else
//    {
//      // Mouse out of widget
//      isMouseOver_ = false;
//    }
//  }
//  else
//  {
//    double x, y;
//    GetPixelCenter(x, y, event);
//    mouseTracker_->MouseMove(x, y);
//    isMouseOver_ = false;
//  }

////  if (isMouseOver_)
////  {
////    UpdateMouseCoordinates(event->x(), event->y());
////  }

//  repaint();
//}


//void QCairoWidget::wheelEvent(QWheelEvent * event)
//{
//  if (mouseTracker_.get() == NULL)
//  {
//#if 0
//    double x = static_cast<double>(event->x()) + 0.5;
//    double y = static_cast<double>(event->y()) + 0.5;
//    mouseTracker_.reset(viewport_.CreateMouseTracker(*scene_, MouseButton_Middle, x, y));

//    switch (event->orientation())
//    {
//      case Qt::Horizontal:
//        x += event->delta();
//        break;

//      case Qt::Vertical:
//        y += event->delta();
//        break;

//      default:
//        break;
//    }

//    mouseTracker_->MouseMove(x, y);
//    mouseTracker_->MouseUp();
//    mouseTracker_.reset(NULL);

//    repaint();
//#else
//    if (event->orientation() == Qt::Vertical)
//    {
//      unsigned int slice = scene_->GetCurrentSlice();

//      if (event->delta() < 0)
//      {
//        if (slice > 0)
//        {
//          scene_->SetCurrentSlice(slice - 1);
//          emit SliceChanged(slice - 1);
//        }
//      }
//      else
//      {
//        if (slice + 1 < scene_->GetSlicesCount())
//        {
//          scene_->SetCurrentSlice(slice + 1);
//          emit SliceChanged(slice + 1);
//        }
//      }

//      repaint();
//    }
//#endif
//  }

//  UpdateMouseCoordinates(event->x(), event->y());
//}



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

//void QCairoWidget::UpdateMouseCoordinates(int x,
//                                         int y)
//{
//  if (scene_ != NULL)
//  {
//    double cx = static_cast<double>(x) + 0.5;
//    double cy = static_cast<double>(y) + 0.5;

//    std::string coordinates;
//    viewport_.FormatCoordinates(coordinates, *scene_, cx, cy);
        
//    emit PositionChanged(QString(coordinates.c_str()));
//  }
//}


//void QCairoWidget::SetDefaultView()
//{
//  viewport_.SetDefaultView();
//  repaint();
//}


//void QCairoWidget::Refresh()
//{
//  if (scene_ != NULL &&
//      updateNeeded_)
//  {
//    updateNeeded_ = false;
//    emit ContentChanged();
//    repaint();
//  }
//}


//void QCairoWidget::SetSlice(int index)
//{
//  if (scene_ != NULL &&
//      index >= 0 &&
//      index < static_cast<int>(scene_->GetSlicesCount()))
//  {
//    scene_->SetCurrentSlice(index);
//    emit SliceChanged(index);
//    repaint();
//  }
//}
