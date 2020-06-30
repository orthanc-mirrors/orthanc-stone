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


#pragma once
#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLContext>

#include <boost/shared_ptr.hpp>
#include "../../Framework/OpenGL/IOpenGLContext.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "../../Framework/Viewport/ViewportBase.h"
#include "../../Applications/Generic/Scene2DInteractor.h"

namespace OrthancStone
{
  class QStoneOpenGlWidget :
      public QOpenGLWidget,
      public OpenGL::IOpenGLContext,
      public ViewportBase
  {
    std::unique_ptr<OrthancStone::OpenGLCompositor> compositor_;
    boost::shared_ptr<Scene2DInteractor> sceneInteractor_;
    QOpenGLContext                        openGlContext_;

  public:
    QStoneOpenGlWidget(QWidget *parent) :
      QOpenGLWidget(parent),
      ViewportBase("QtStoneOpenGlWidget")  // TODO: we shall be able to define a name but construction time is too early !
    {
      setFocusPolicy(Qt::StrongFocus);  // to enable keyPressEvent
      setMouseTracking(true);           // to enable mouseMoveEvent event when no button is pressed
    }

    void Init()
    {
      QSurfaceFormat requestedFormat;
      requestedFormat.setVersion( 2, 0 );
      openGlContext_.setFormat( requestedFormat );
      openGlContext_.create();
      openGlContext_.makeCurrent(context()->surface());

      compositor_.reset(new OpenGLCompositor(*this, GetScene()));
    }

  protected:

    //**** QWidget overrides
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent* event) override;

    //**** IOpenGLContext overrides

    virtual void MakeCurrent() override;
    virtual void SwapBuffer() override {}

    virtual unsigned int GetCanvasWidth() const override
    {
      return this->width();
    }

    virtual unsigned int GetCanvasHeight() const override
    {
      return this->height();
    }

  public:

    void SetInteractor(boost::shared_ptr<Scene2DInteractor> sceneInteractor)
    {
      sceneInteractor_ = sceneInteractor;
    }

    virtual ICompositor& GetCompositor()
    {
      return *compositor_;
    }

  protected:
    void mouseEvent(QMouseEvent* qtEvent, OrthancStone::GuiAdapterHidEventType guiEventType);
    bool keyEvent(QKeyEvent* qtEvent, OrthancStone::GuiAdapterHidEventType guiEventType);

  };
}
