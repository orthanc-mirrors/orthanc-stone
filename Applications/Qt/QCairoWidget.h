#pragma once

#include "../../Framework/Widgets/CairoWidget.h"
#include "../../Applications/Generic/BasicNativeApplicationContext.h"
#include "../../Framework/Viewport/CairoSurface.h"

#include <QWidget>
#include <QGestureEvent>
#include <memory>
#include <cassert>

class QCairoWidget : public QWidget
{
  Q_OBJECT

private:
  std::auto_ptr<QImage>         image_;
  OrthancStone::CairoSurface   surface_;
  OrthancStone::BasicNativeApplicationContext* context_;
  bool                          isMouseOver_;
  int                           mouseOverX_;
  int                           mouseOverY_;
  bool                          updateNeeded_;

  std::auto_ptr<OrthancStone::IMouseTracker>  mouseTracker_;

//  void UpdateMouseCoordinates(int x,
//                              int y);

protected:
  void paintEvent(QPaintEvent *event);

  void resizeEvent(QResizeEvent *event);

//  void mouseMoveEvent(QMouseEvent *event);

//  void mousePressEvent(QMouseEvent *event);

//  void mouseReleaseEvent(QMouseEvent *event);

//  void wheelEvent(QWheelEvent *event);

public:
  explicit QCairoWidget(QWidget *parent);
 
  virtual ~QCairoWidget();

  void SetContext(OrthancStone::BasicNativeApplicationContext& context)
  {
    context_ = &context;
  }

//  void SetCentralWidget(OrthancStone::CairoWidget& widget)
//  {
//    centralWidget_ = &widget;
//  }

//  void SetScene(OrthancStone::ICairoScene& scene);

//  bool HasScene() const
//  {
//    return scene_ != NULL;
//  }

//  OrthancStone::ICairoScene& GetScene() const
//  {
//    assert(HasScene());
//    return *scene_;
//  }

//  virtual void SignalUpdatedScene(OrthancStone::ICairoScene& scene)
//  {
//    //printf("UPDATE NEEDED\n"); fflush(stdout);
//    updateNeeded_ = true;
//  }

signals:
//  void SliceChanged(int position);
  
//  void RangeChanged(unsigned int countSlices);
  
//  void PositionChanged(const QString& position);

//  void ContentChanged();
                                               
public slots:
//  void SetDefaultView();

//  void Refresh();

//  void SetSlice(int index);
};
