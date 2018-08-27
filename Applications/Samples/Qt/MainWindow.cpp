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
  ui_->setupUi(this);
  cairoCentralWidget_ = ui_->cairoCentralWidget;
  cairoCentralWidget_->SetContext(context_);
}

MainWindow::~MainWindow()
{
  delete ui_;
}


