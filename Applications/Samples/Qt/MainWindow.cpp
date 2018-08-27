#include "MainWindow.h"

/**
 * Don't use "ui_MainWindow.h" instead of <ui_MainWindow.h> below, as
 * this makes CMake unable to detect when the UI file changes.
 **/
#include <ui_MainWindow.h>

MainWindow::MainWindow(OrthancStone::BasicNativeApplicationContext& context, QWidget *parent) :
  QMainWindow(parent),
  ui_(new Ui::MainWindow),
  context_(context)
{
  refreshTimer_ = new QTimer(this);
  ui_->setupUi(this);
  cairoCentralWidget_ = ui_->cairoCentralWidget;
  cairoCentralWidget_->SetContext(context_);

  //connect(refreshTimer_, SIGNAL(timeout()), ui_->ca, SLOT(Refresh()));
  refreshTimer_->start(100);
}

MainWindow::~MainWindow()
{
  delete ui_;
  delete refreshTimer_;
}


