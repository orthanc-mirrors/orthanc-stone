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

#include "SampleMainWindow.h"

/**
 * Don't use "ui_MainWindow.h" instead of <ui_MainWindow.h> below, as
 * this makes CMake unable to detect when the UI file changes.
 **/
#include <ui_SampleMainWindow.h>
#include "../../Applications/Samples/SampleApplicationBase.h"

namespace OrthancStone
{
  namespace Samples
  {

    SampleMainWindow::SampleMainWindow(OrthancStone::NativeStoneApplicationContext& context, OrthancStone::Samples::SampleApplicationBase& stoneSampleApplication, QWidget *parent) :
      QStoneMainWindow(context, parent),
      ui_(new Ui::SampleMainWindow),
      stoneSampleApplication_(stoneSampleApplication)
    {
      ui_->setupUi(this);
      SetCentralStoneWidget(ui_->cairoCentralWidget);

#if QT_VERSION >= 0x050000
      connect(ui_->toolButton1, &QToolButton::clicked, this, &SampleMainWindow::tool1Clicked);
      connect(ui_->toolButton2, &QToolButton::clicked, this, &SampleMainWindow::tool2Clicked);
      connect(ui_->pushButton1, &QPushButton::clicked, this, &SampleMainWindow::pushButton1Clicked);
      connect(ui_->pushButton1, &QPushButton::clicked, this, &SampleMainWindow::pushButton2Clicked);
#else
      connect(ui_->toolButton1, SIGNAL(clicked()), this, SLOT(tool1Clicked()));
      connect(ui_->toolButton2, SIGNAL(clicked()), this, SLOT(tool2Clicked()));
      connect(ui_->pushButton1, SIGNAL(clicked()), this, SLOT(pushButton1Clicked()));
      connect(ui_->pushButton1, SIGNAL(clicked()), this, SLOT(pushButton2Clicked()));
#endif

      std::string pushButton1Name;
      std::string pushButton2Name;
      std::string tool1Name;
      std::string tool2Name;
      stoneSampleApplication_.GetButtonNames(pushButton1Name, pushButton2Name, tool1Name, tool2Name);

      ui_->toolButton1->setText(QString::fromStdString(tool1Name));
      ui_->toolButton2->setText(QString::fromStdString(tool2Name));
      ui_->pushButton1->setText(QString::fromStdString(pushButton1Name));
      ui_->pushButton2->setText(QString::fromStdString(pushButton2Name));
    }

    SampleMainWindow::~SampleMainWindow()
    {
      delete ui_;
    }

    void SampleMainWindow::tool1Clicked()
    {
      stoneSampleApplication_.OnTool1Clicked();
    }

    void SampleMainWindow::tool2Clicked()
    {
      stoneSampleApplication_.OnTool2Clicked();
    }

    void SampleMainWindow::pushButton1Clicked()
    {
      stoneSampleApplication_.OnPushButton1Clicked();
    }

    void SampleMainWindow::pushButton2Clicked()
    {
      stoneSampleApplication_.OnPushButton2Clicked();
    }

  }
}
