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

#include "../../Framework/Toolbox/IWebService.h"
#include "Oracle.h"
#include "WebServiceGetCommand.h"
#include "WebServicePostCommand.h"
#include "../../Applications/Generic/NativeStoneApplicationContext.h"

namespace OrthancStone
{
  // The OracleWebService performs HTTP requests in a native environment.
  // It uses a thread pool to handle multiple HTTP requests in a same time.
  // It works asynchronously to mimick the behaviour of the WebService running in a WASM environment.
  class OracleWebService : public IWebService
  {
  private:
    Oracle&                        oracle_;
    NativeStoneApplicationContext& context_;
    Orthanc::WebServiceParameters  parameters_;

  public:
    OracleWebService(MessageBroker& broker,
                     Oracle& oracle,
                     const Orthanc::WebServiceParameters& parameters,
                     NativeStoneApplicationContext& context) :
      IWebService(broker),
      oracle_(oracle),
      context_(context),
      parameters_(parameters)
    {
    }

    virtual void GetAsync(const std::string& uri,
                          const Headers& headers,
                          Orthanc::IDynamicObject* payload, // takes ownership
                          MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,   // takes ownership
                          MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,// takes ownership
                          unsigned int timeoutInSeconds = 60)
    {
      oracle_.Submit(new WebServiceGetCommand(broker_, successCallback, failureCallback, parameters_, uri, headers, timeoutInSeconds, payload, context_));
    }

    virtual void PostAsync(const std::string& uri,
                           const Headers& headers,
                           const std::string& body,
                           Orthanc::IDynamicObject* payload, // takes ownership
                           MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback, // takes ownership
                           MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL, // takes ownership
                           unsigned int timeoutInSeconds = 60)
    {
      oracle_.Submit(new WebServicePostCommand(broker_, successCallback, failureCallback, parameters_, uri, headers, timeoutInSeconds, body, payload, context_));
    }

    void Start()
    {
      oracle_.Start();
    }

    void Stop()
    {
      oracle_.Stop();
    }
  };
}
