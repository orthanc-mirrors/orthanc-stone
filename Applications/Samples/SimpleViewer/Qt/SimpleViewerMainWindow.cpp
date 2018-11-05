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

#include "SimpleViewerMainWindow.h"

/**
 * Don't use "ui_MainWindow.h" instead of <ui_MainWindow.h> below, as
 * this makes CMake unable to detect when the UI file changes.
 **/
#include <ui_SimpleViewerMainWindow.h>
#include "../SimpleViewerApplication.h"

namespace SimpleViewer
{

  SimpleViewerMainWindow::SimpleViewerMainWindow(OrthancStone::NativeStoneApplicationContext& context, SimpleViewerApplication& stoneApplication, QWidget *parent) :
    QStoneMainWindow(context, parent),
    ui_(new Ui::SimpleViewerMainWindow),
    stoneApplication_(stoneApplication)
  {
    ui_->setupUi(this);
    SetCentralStoneWidget(ui_->cairoCentralWidget);

#if QT_VERSION >= 0x050000
    connect(ui_->toolButtonCrop, &QToolButton::clicked, this, &SimpleViewerMainWindow::cropClicked);
    connect(ui_->pushButtonUndoCrop, &QToolButton::clicked, this, &SimpleViewerMainWindow::undoCropClicked);
    connect(ui_->toolButtonLine, &QToolButton::clicked, this, &SimpleViewerMainWindow::lineClicked);
    connect(ui_->toolButtonCircle, &QToolButton::clicked, this, &SimpleViewerMainWindow::circleClicked);
    connect(ui_->toolButtonWindowing, &QToolButton::clicked, this, &SimpleViewerMainWindow::windowingClicked);
    connect(ui_->pushButtonRotate, &QPushButton::clicked, this, &SimpleViewerMainWindow::rotateClicked);
    connect(ui_->pushButtonInvert, &QPushButton::clicked, this, &SimpleViewerMainWindow::invertClicked);
#else
    connect(ui_->toolButtonCrop, SIGNAL(clicked()), this, SLOT(cropClicked()));
    connect(ui_->toolButtonLine, SIGNAL(clicked()), this, SLOT(lineClicked()));
    connect(ui_->toolButtonCircle, SIGNAL(clicked()), this, SLOT(circleClicked()));
    connect(ui_->toolButtonWindowing, SIGNAL(clicked()), this, SLOT(windowingClicked()));
    connect(ui_->pushButtonUndoCrop, SIGNAL(clicked()), this, SLOT(undoCropClicked()));
    connect(ui_->pushButtonRotate, SIGNAL(clicked()), this, SLOT(rotateClicked()));
    connect(ui_->pushButtonInvert, SIGNAL(clicked()), this, SLOT(invertClicked()));
#endif
  }

  SimpleViewerMainWindow::~SimpleViewerMainWindow()
  {
    delete ui_;
  }

  void SimpleViewerMainWindow::cropClicked()
  {
    GenericNoArgCommand command("selectTool:crop");
    stoneApplication_.ExecuteCommand(command);
  }

  void SimpleViewerMainWindow::undoCropClicked()
  {
    GenericNoArgCommand command("action:undo-crop");
    stoneApplication_.ExecuteCommand(command);
  }

  void SimpleViewerMainWindow::lineClicked()
  {
    GenericNoArgCommand command("selectTool:line-measure");
    stoneApplication_.ExecuteCommand(command);
  }

  void SimpleViewerMainWindow::circleClicked()
  {
    GenericNoArgCommand command("selectTool:circle-measure");
    stoneApplication_.ExecuteCommand(command);
  }

  void SimpleViewerMainWindow::windowingClicked()
  {
    GenericNoArgCommand command("selectTool:windowing");
    stoneApplication_.ExecuteCommand(command);
  }

  void SimpleViewerMainWindow::rotateClicked()
  {
    GenericNoArgCommand command("action:rotate");
    stoneApplication_.ExecuteCommand(command);
  }

  void SimpleViewerMainWindow::invertClicked()
  {
    GenericNoArgCommand command("action:invert");
    stoneApplication_.ExecuteCommand(command);
  }
}
