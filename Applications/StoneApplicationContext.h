/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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
#include "../Framework/Toolbox/IDelayedCallExecutor.h"
#include "../Framework/Toolbox/OrthancApiClient.h"
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
  private:
    MessageBroker&                   broker_;
    IWebService*                     webService_;
    IDelayedCallExecutor*            delayedCallExecutor_;
    std::auto_ptr<OrthancApiClient>  orthanc_;
    std::string                      orthancBaseUrl_;

    void InitializeOrthanc();

  public:
    StoneApplicationContext(MessageBroker& broker) :
      broker_(broker),
      webService_(NULL),
      delayedCallExecutor_(NULL)
    {
    }

    virtual ~StoneApplicationContext()
    {
    }

    MessageBroker& GetMessageBroker()
    {
      return broker_;
    }

    bool HasWebService() const
    {
      return webService_ != NULL;
    }

    IWebService& GetWebService();

    OrthancApiClient& GetOrthancApiClient();

    void SetWebService(IWebService& webService);

    void SetOrthancBaseUrl(const std::string& baseUrl);

    void SetDelayedCallExecutor(IDelayedCallExecutor& delayedCallExecutor)
    {
      delayedCallExecutor_ = &delayedCallExecutor;
    }

    IDelayedCallExecutor& GetDelayedCallExecutor()
    {
      return *delayedCallExecutor_;
    }
  };
}
