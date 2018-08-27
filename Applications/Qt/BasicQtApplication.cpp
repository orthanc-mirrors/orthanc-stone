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


#if ORTHANC_ENABLE_QT != 1
#error this file shall be included only with the ORTHANC_ENABLE_QT set to 1
#endif

#include "BasicQtApplication.h"
#include <boost/program_options.hpp>
#include <QApplication>

#include "../../Framework/Toolbox/MessagingToolbox.h"

#include <Core/Logging.h>
#include <Core/HttpClient.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/OrthancHttpConnection.h>
#include "../../Platforms/Generic/OracleWebService.h"
#include "../../Applications/Samples/Qt/MainWindow.h"


namespace OrthancStone
{
  void BasicQtApplication::Initialize()
  {
  }

  void BasicQtApplication::DeclareCommandLineOptions(boost::program_options::options_description& options)
  {
  }

  void BasicQtApplication::Run(BasicNativeApplicationContext& context, const std::string& title, int argc, char* argv[])
  {
    context.Start();

    QApplication app(argc, argv);
    MainWindow window(context);

    window.show();
    app.exec();

    context.Stop();
  }

  void BasicQtApplication::Finalize()
  {
  }


}
