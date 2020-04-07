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
