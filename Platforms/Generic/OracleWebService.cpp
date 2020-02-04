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


#include "OracleWebService.h"
#include "../../Framework/Deprecated/Toolbox/IWebService.h"

namespace Deprecated
{


  class OracleWebService::WebServiceCachedGetCommand : public IOracleCommand, OrthancStone::IObservable
  {
  protected:
    std::auto_ptr<OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage> >  successCallback_;
    std::auto_ptr<Orthanc::IDynamicObject>                                  payload_;
    boost::shared_ptr<BaseWebService::CachedHttpRequestSuccessMessage>      cachedMessage_;
    OrthancStone::NativeStoneApplicationContext&                                          context_;

  public:
    WebServiceCachedGetCommand(OrthancStone::MessageBroker& broker,
                               OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,  // takes ownership
                               boost::shared_ptr<BaseWebService::CachedHttpRequestSuccessMessage> cachedMessage,
                               Orthanc::IDynamicObject* payload /* takes ownership */,
                               OrthancStone::NativeStoneApplicationContext& context
                               ) :
      IObservable(broker),
      successCallback_(successCallback),
      payload_(payload),
      cachedMessage_(cachedMessage),
      context_(context)
    {
    }

    virtual void Execute()
    {
      // nothing to do, everything is in the commit
    }

    virtual void Commit()
    {
      // We want to make sure that, i.e, the UpdateThread is not
      // triggered while we are updating the "model" with the result of
      // a WebServiceCommand
      OrthancStone::NativeStoneApplicationContext::GlobalMutexLocker lock(context_);

      IWebService::HttpRequestSuccessMessage successMessage(cachedMessage_->GetUri(),
                                                            cachedMessage_->GetAnswer(),
                                                            cachedMessage_->GetAnswerSize(),
                                                            cachedMessage_->GetAnswerHttpHeaders(),
                                                            payload_.get());

      successCallback_->Apply(successMessage);
    }
  };

  void OracleWebService::NotifyHttpSuccessLater(boost::shared_ptr<BaseWebService::CachedHttpRequestSuccessMessage> cachedMessage,
                                                Orthanc::IDynamicObject* payload, // takes ownership
                                                OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback)
  {
    oracle_.Submit(new WebServiceCachedGetCommand(GetBroker(), successCallback, cachedMessage, payload, context_));
  }


}
