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


#include "WebServiceCommandBase.h"

#include <Core/HttpClient.h>

namespace OrthancStone
{
  WebServiceCommandBase::WebServiceCommandBase(MessageBroker& broker,
                                               IWebService::ICallback& callback,
                                               const Orthanc::WebServiceParameters& parameters,
                                               const std::string& uri,
                                               const IWebService::Headers& headers,
                                               Orthanc::IDynamicObject* payload /* takes ownership */,
                                               BasicSdlApplicationContext& context) :
    IObservable(broker),
    callback_(callback),
    parameters_(parameters),
    uri_(uri),
    headers_(headers),
    payload_(payload),
    context_(context)
  {
    DeclareEmittableMessage(MessageType_HttpRequestError);
    DeclareEmittableMessage(MessageType_HttpRequestSuccess);
    RegisterObserver(callback);
  }


  void WebServiceCommandBase::Commit()
  {
    BasicSdlApplicationContext::GlobalMutexLocker lock(context_);  // we want to make sure that, i.e, the UpdateThread is not triggered while we are updating the "model" with the result of a WebServiceCommand

    if (success_)
    {
      IWebService::ICallback::HttpRequestSuccessMessage message(uri_, answer_.c_str(), answer_.size(), payload_.release());
      EmitMessage(message);
    }
    else
    {
      IWebService::ICallback::HttpRequestErrorMessage message(uri_, payload_.release());
      EmitMessage(message);
    }
  }
}
