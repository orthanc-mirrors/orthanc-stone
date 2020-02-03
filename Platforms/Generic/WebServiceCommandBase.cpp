/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "WebServiceCommandBase.h"

#include <Core/HttpClient.h>

namespace Deprecated
{
  WebServiceCommandBase::WebServiceCommandBase(OrthancStone::MessageBroker& broker,
                                               OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                                               OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                               const Orthanc::WebServiceParameters& parameters,
                                               const std::string& url,
                                               const IWebService::HttpHeaders& headers,
                                               unsigned int timeoutInSeconds,
                                               Orthanc::IDynamicObject* payload /* takes ownership */,
                                               OrthancStone::NativeStoneApplicationContext& context) :
    IObservable(broker),
    successCallback_(successCallback),
    failureCallback_(failureCallback),
    parameters_(parameters),
    url_(url),
    headers_(headers),
    payload_(payload),
    success_(false),
    httpStatus_(Orthanc::HttpStatus_None),
    context_(context),
    timeoutInSeconds_(timeoutInSeconds)
  {
  }


  void WebServiceCommandBase::Commit()
  {
    // We want to make sure that, i.e, the UpdateThread is not
    // triggered while we are updating the "model" with the result of
    // a WebServiceCommand
    OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker lock(context_); 

    if (success_ && successCallback_.get() != NULL)
    {
      IWebService::HttpRequestSuccessMessage message
        (url_, answer_.c_str(), answer_.size(), answerHeaders_, payload_.get());
      successCallback_->Apply(message);
    }
    else if (!success_ && failureCallback_.get() != NULL)
    {
      IWebService::HttpRequestErrorMessage message(url_, httpStatus_, payload_.get());
      failureCallback_->Apply(message);
    }
  }
}
