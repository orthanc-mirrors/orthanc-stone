/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
4 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#pragma once

#include "../../Applications/Generic/NativeStoneApplicationContext.h"
#include "../../Framework/Viewport/CairoSurface.h"
#include "../../Framework/Widgets/IWidget.h"

#include <QWidget>
#include <memory>
#include <cassert>

class QCairoWidget : public QWidget
{
  Q_OBJECT

private:
  class StoneObserver : public OrthancStone::IObserver
  {
  private:
    QCairoWidget& that_;
    
  public:
    StoneObserver(QCairoWidget& that,
                  Deprecated::IViewport& viewport,
                  OrthancStone::MessageBroker& broker);

    void OnViewportChanged(const Deprecated::IViewport::ViewportChangedMessage& message)
    {
      that_.OnViewportChanged();
    }
  };
  
  std::auto_ptr<QImage>         image_;
  OrthancStone::CairoSurface    surface_;
  OrthancStone::NativeStoneApplicationContext* context_;
  std::auto_ptr<StoneObserver>  observer_;

protected:
  virtual void paintEvent(QPaintEvent *event);

  virtual void resizeEvent(QResizeEvent *event);

  virtual void mouseMoveEvent(QMouseEvent *event);

  virtual void mousePressEvent(QMouseEvent *event);

  virtual void mouseReleaseEvent(QMouseEvent *event);

  virtual void wheelEvent(QWheelEvent *event);

  virtual void keyPressEvent(QKeyEvent *event);

public:
  explicit QCairoWidget(QWidget *parent);
 
  void SetContext(OrthancStone::NativeStoneApplicationContext& context);

  void OnViewportChanged()
  {
    update();  // schedule a repaint (handled by Qt)
    emit ContentChanged();
  }

signals:
  void ContentChanged();
                                               
public slots:

};
