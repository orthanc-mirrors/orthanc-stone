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


#include "WebServiceDeleteCommand.h"

#include <Core/HttpClient.h>

namespace Deprecated
{
  WebServiceDeleteCommand::WebServiceDeleteCommand(OrthancStone::MessageBroker& broker,
                                                   OrthancStone::MessageHandler<Deprecated::IWebService::HttpRequestSuccessMessage>* successCallback,  // takes ownership
                                                   OrthancStone::MessageHandler<Deprecated::IWebService::HttpRequestErrorMessage>* failureCallback,  // takes ownership
                                                   const Orthanc::WebServiceParameters& parameters,
                                                   const std::string& url,
                                                   const Deprecated::IWebService::HttpHeaders& headers,
                                                   unsigned int timeoutInSeconds,
                                                   Orthanc::IDynamicObject* payload /* takes ownership */,
                                                   OrthancStone::NativeStoneApplicationContext& context) :
    WebServiceCommandBase(broker, successCallback, failureCallback, parameters, url, headers, timeoutInSeconds, payload, context)
  {
  }

  void WebServiceDeleteCommand::Execute()
  {
    Orthanc::HttpClient client(parameters_, "/");
    client.SetUrl(url_);
    client.SetTimeout(timeoutInSeconds_);
    client.SetMethod(Orthanc::HttpMethod_Delete);

    for (Deprecated::IWebService::HttpHeaders::const_iterator it = headers_.begin(); it != headers_.end(); it++ )
    {
      client.AddHeader(it->first, it->second);
    }

    success_ = client.Apply(answer_, answerHeaders_);
  }

}
