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

#include "IWebService.h"

#include <string>
#include <map>

namespace Deprecated
{
  // This is an intermediate of IWebService that implements some caching on
  // the HTTP GET requests
  class BaseWebService : public IWebService, public OrthancStone::IObserver
  {
  public:
    class CachedHttpRequestSuccessMessage
    {
    protected:
      std::string                    uri_;
      void*                          answer_;
      size_t                         answerSize_;
      IWebService::HttpHeaders       answerHeaders_;

    public:
      CachedHttpRequestSuccessMessage(const IWebService::HttpRequestSuccessMessage& message) :
        uri_(message.GetUri()),
        answerSize_(message.GetAnswerSize()),
        answerHeaders_(message.GetAnswerHttpHeaders())
      {
        answer_ =  malloc(answerSize_);
        memcpy(answer_, message.GetAnswer(), answerSize_);
      }

      ~CachedHttpRequestSuccessMessage()
      {
        free(answer_);
      }

      const std::string& GetUri() const
      {
        return uri_;
      }

      const void* GetAnswer() const
      {
        return answer_;
      }

      size_t GetAnswerSize() const
      {
        return answerSize_;
      }

      const IWebService::HttpHeaders&  GetAnswerHttpHeaders() const
      {
        return answerHeaders_;
      }

    };
  protected:
    class BaseWebServicePayload;

    bool          cacheEnabled_;
    std::map<std::string, boost::shared_ptr<CachedHttpRequestSuccessMessage> > cache_;  // TODO: this is currently an infinite cache !

  public:

    BaseWebService(OrthancStone::MessageBroker& broker) :
      IWebService(broker),
      IObserver(broker),
      cacheEnabled_(true)
    {
    }

    virtual ~BaseWebService()
    {
    }

    virtual void EnableCache(bool enable)
    {
      cacheEnabled_ = enable;
    }

    virtual void GetAsync(const std::string& uri,
                          const HttpHeaders& headers,
                          Orthanc::IDynamicObject* payload  /* takes ownership */,
                          OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                          OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                          unsigned int timeoutInSeconds = 60);

  protected:
    virtual void GetAsyncInternal(const std::string& uri,
                          const HttpHeaders& headers,
                          Orthanc::IDynamicObject* payload  /* takes ownership */,
                          OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                          OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                          unsigned int timeoutInSeconds = 60) = 0;

    virtual void NotifyHttpSuccessLater(boost::shared_ptr<BaseWebService::CachedHttpRequestSuccessMessage> cachedHttpMessage,
                                        Orthanc::IDynamicObject* payload, // takes ownership
                                        OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback) = 0;

  private:
    void NotifyHttpSuccess(const IWebService::HttpRequestSuccessMessage& message);

    void NotifyHttpError(const IWebService::HttpRequestErrorMessage& message);

    void CacheAndNotifyHttpSuccess(const IWebService::HttpRequestSuccessMessage& message);

  };
}