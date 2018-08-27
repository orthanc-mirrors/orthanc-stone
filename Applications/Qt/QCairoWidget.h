#pragma once

#include "../../Framework/Widgets/CairoWidget.h"
#include "../../Applications/Generic/BasicNativeApplicationContext.h"
#include "../../Framework/Viewport/CairoSurface.h"

#include <QWidget>
#include <QGestureEvent>
#include <memory>
#include <cassert>

class QCairoWidget : public QWidget, public OrthancStone::IViewport::IObserver
{
  Q_OBJECT

private:
  std::auto_ptr<QImage>         image_;
  OrthancStone::CairoSurface    surface_;
  OrthancStone::BasicNativeApplicationContext* context_;

protected:
  virtual void paintEvent(QPaintEvent *event);

  virtual void resizeEvent(QResizeEvent *event);

  virtual void mouseMoveEvent(QMouseEvent *event);

  virtual void mousePressEvent(QMouseEvent *event);

  virtual void mouseReleaseEvent(QMouseEvent *event);

  virtual void wheelEvent(QWheelEvent *event);

public:
  explicit QCairoWidget(QWidget *parent);
 
  virtual ~QCairoWidget();

  void SetContext(OrthancStone::BasicNativeApplicationContext& context);

  virtual void OnViewportContentChanged(const OrthancStone::IViewport& /*sceneNotUsed*/)
  {
    update();  // schedule a repaint (handled by Qt)
    emit ContentChanged();
  }

signals:

  void ContentChanged();
                                               
public slots:

};
