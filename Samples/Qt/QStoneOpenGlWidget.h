#pragma once
#include "../../Framework/OpenGL/OpenGLIncludes.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <boost/shared_ptr.hpp>
#include "../../Framework/OpenGL/IOpenGLContext.h"
#include "../../Framework/Scene2D/OpenGLCompositor.h"


class QStoneOpenGlWidget : public QOpenGLWidget, public OrthancStone::OpenGL::IOpenGLContext
{
  boost::shared_ptr<OrthancStone::OpenGLCompositor> compositor_;

public:
  QStoneOpenGlWidget(QWidget *parent) : QOpenGLWidget(parent) { }

protected:
  void initializeGL() override;

  void resizeGL(int w, int h) override;

  void paintGL() override;

  virtual void MakeCurrent() override {}

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
  void SetCompositor(boost::shared_ptr<OrthancStone::OpenGLCompositor> compositor)
  {
    compositor_ = compositor;
  }

};
