#pragma once
#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <boost/shared_ptr.hpp>
#include "../../Framework/OpenGL/IOpenGLContext.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"
#include "Scene2DInteractor.h"

namespace OrthancStone
{
  class QStoneOpenGlWidget : public QOpenGLWidget, public OrthancStone::OpenGL::IOpenGLContext
  {
    boost::shared_ptr<OrthancStone::OpenGLCompositor> compositor_;
    boost::shared_ptr<Scene2DInteractor> sceneInteractor_;

  public:
    QStoneOpenGlWidget(QWidget *parent) :
      QOpenGLWidget(parent)
    {
    }

  protected:

    //**** QWidget overrides
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

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

    void SetCompositor(boost::shared_ptr<OrthancStone::OpenGLCompositor> compositor)
    {
      compositor_ = compositor;
    }

  protected:
    void mouseEvent(QMouseEvent* qtEvent, OrthancStone::GuiAdapterHidEventType guiEventType);

  };
}
