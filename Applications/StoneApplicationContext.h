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

#include "../Framework/Deprecated/Toolbox/IWebService.h"
#include "../Framework/Deprecated/Toolbox/IDelayedCallExecutor.h"
#include "../Framework/Deprecated/Toolbox/OrthancApiClient.h"
#include "../Framework/Deprecated/Viewport/WidgetViewport.h"


#ifdef _MSC_VER
  #if _MSC_VER > 1910
    #define orthanc_override override
  #else
    #define orthanc_override
  #endif
#elif defined __GNUC__
  #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
/* Test for GCC > 3.2.0 */
  #if GCC_VERSION > 40900
    #define orthanc_override override
  #else
    #define orthanc_override
  #endif
#else
    #define orthanc_override
#endif

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
    Deprecated::IWebService*         webService_;
    Deprecated::IDelayedCallExecutor*            delayedCallExecutor_;
    std::auto_ptr<Deprecated::OrthancApiClient>  orthanc_;
    std::string                      orthancBaseUrl_;

    void InitializeOrthanc();

  public:
    StoneApplicationContext() :
      webService_(NULL),
      delayedCallExecutor_(NULL)
    {
    }

    virtual ~StoneApplicationContext()
    {
    }

    bool HasWebService() const
    {
      return webService_ != NULL;
    }

    Deprecated::IWebService& GetWebService();

    Deprecated::OrthancApiClient& GetOrthancApiClient();

    void SetWebService(Deprecated::IWebService& webService);

    void SetOrthancBaseUrl(const std::string& baseUrl);

    void SetDelayedCallExecutor(Deprecated::IDelayedCallExecutor& delayedCallExecutor)
    {
      delayedCallExecutor_ = &delayedCallExecutor;
    }

    Deprecated::IDelayedCallExecutor& GetDelayedCallExecutor()
    {
      return *delayedCallExecutor_;
    }
  };
}
