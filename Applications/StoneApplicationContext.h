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


#pragma once

#include "../Framework/Toolbox/IWebService.h"
#include "../Framework/Viewport/WidgetViewport.h"

#include <list>

namespace OrthancStone
{
  // a StoneApplicationContext contains the services that a StoneApplication
  // uses and that depends on the environment in which the Application executes.
  // I.e, the StoneApplicationContext provides a WebService interface such that
  // the StoneApplication can perform HTTP requests.  In a WASM environment,
  // the WebService is provided by the browser while, in a native environment,
  // the WebService is provided by the OracleWebService (a C++ Http client)
  class StoneApplicationContext : public boost::noncopyable
  {

  protected:
    IWebService* webService_;
  public:
    StoneApplicationContext()
      : webService_(NULL)
    {
    }

    IWebService& GetWebService() {return *webService_;}
    void SetWebService(IWebService& webService)
    {
      webService_ = &webService;
    }

    virtual ~StoneApplicationContext() {}
  };
}