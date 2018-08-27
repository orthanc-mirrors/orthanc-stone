#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "../../Qt/QCairoWidget.h"
#include "../../Generic/BasicNativeApplicationContext.h"
namespace Ui 
{
  class MainWindow;
}


class MainWindow : public QMainWindow
{
  Q_OBJECT

public:

private:
  Ui::MainWindow        *ui_;
  QTimer                *refreshTimer_;
  OrthancStone::BasicNativeApplicationContext& context_;
  QCairoWidget          *cairoCentralWidget_;

public:
  explicit MainWindow(OrthancStone::BasicNativeApplicationContext& context, QWidget *parent = 0);
  ~MainWindow();
};

#endif // MAINWINDOW_H
