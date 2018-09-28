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

#include "IOracleCommand.h"

#include "../../Framework/Toolbox/IWebService.h"
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Messages/ICallable.h"
#include "../../Applications/Generic/NativeStoneApplicationContext.h"

#include <Core/WebServiceParameters.h>

#include <memory>

namespace OrthancStone
{
  class WebServiceCommandBase : public IOracleCommand, IObservable
  {
  protected:
    std::auto_ptr<MessageHandler<IWebService::HttpRequestSuccessMessage>>                              successCallback_;
    std::auto_ptr<MessageHandler<IWebService::HttpRequestErrorMessage>>                                failureCallback_;
    Orthanc::WebServiceParameters           parameters_;
    std::string                             uri_;
    std::map<std::string, std::string>      headers_;
    std::auto_ptr<Orthanc::IDynamicObject>  payload_;
    bool                                    success_;
    std::string                             answer_;
    NativeStoneApplicationContext&          context_;
    unsigned int                            timeoutInSeconds_;

  public:
    WebServiceCommandBase(MessageBroker& broker,
                          MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,  // takes ownership
                          MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,  // takes ownership
                          const Orthanc::WebServiceParameters& parameters,
                          const std::string& uri,
                          const std::map<std::string, std::string>& headers,
                          unsigned int timeoutInSeconds,
                          Orthanc::IDynamicObject* payload /* takes ownership */,
                          NativeStoneApplicationContext& context
                          );

    virtual void Execute() = 0;

    virtual void Commit();
  };

}
