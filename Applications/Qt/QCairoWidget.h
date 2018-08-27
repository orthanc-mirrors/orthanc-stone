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
