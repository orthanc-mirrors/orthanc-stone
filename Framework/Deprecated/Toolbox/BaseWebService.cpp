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


#include "BaseWebService.h"

#include "../../Messages/IObservable.h"
#include "../../../Platforms/Generic/IOracleCommand.h"

#include <Core/OrthancException.h>

#include <boost/shared_ptr.hpp>
#include <algorithm>

namespace Deprecated
{


  class BaseWebService::BaseWebServicePayload : public Orthanc::IDynamicObject
  {
  private:
    std::auto_ptr< OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage> >   userSuccessHandler_;
    std::auto_ptr< OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage> >     userFailureHandler_;
    std::auto_ptr< Orthanc::IDynamicObject>                                   userPayload_;

  public:
    BaseWebServicePayload(OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* userSuccessHandler,
                          OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* userFailureHandler,
                          Orthanc::IDynamicObject* userPayload) :
      userSuccessHandler_(userSuccessHandler),
      userFailureHandler_(userFailureHandler),
      userPayload_(userPayload)
    {
    }

    void HandleSuccess(const IWebService::HttpRequestSuccessMessage& message) const
    {
      if (userSuccessHandler_.get() != NULL)
      {
        // recreate a success message with the user payload
        IWebService::HttpRequestSuccessMessage successMessage(message.GetUri(),
                                                              message.GetAnswer(),
                                                              message.GetAnswerSize(),
                                                              message.GetAnswerHttpHeaders(),
                                                              userPayload_.get());
        userSuccessHandler_->Apply(successMessage);
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    void HandleFailure(const IWebService::HttpRequestErrorMessage& message) const
    {
      if (userFailureHandler_.get() != NULL)
      {
        // recreate a failure message with the user payload
        IWebService::HttpRequestErrorMessage failureMessage(message.GetUri(),
                                                            userPayload_.get());

        userFailureHandler_->Apply(failureMessage);
      }
    }

  };


  void BaseWebService::GetAsync(const std::string& uri,
                                const HttpHeaders& headers,
                                Orthanc::IDynamicObject* payload  /* takes ownership */,
                                OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                                OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                unsigned int timeoutInSeconds)
  {
    if (cache_.find(uri) == cache_.end())
    {
      GetAsyncInternal(uri, headers,
                       new BaseWebService::BaseWebServicePayload(successCallback, failureCallback, payload), // ownership is transfered
                       new OrthancStone::Callable<BaseWebService, IWebService::HttpRequestSuccessMessage>
                       (*this, &BaseWebService::CacheAndNotifyHttpSuccess),
                       new OrthancStone::Callable<BaseWebService, IWebService::HttpRequestErrorMessage>
                       (*this, &BaseWebService::NotifyHttpError),
                       timeoutInSeconds);
    }
    else
    {
      // put the uri on top of the most recently accessed list
      std::deque<std::string>::iterator it = std::find(orderedCacheKeys_.begin(), orderedCacheKeys_.end(), uri);
      if (it != orderedCacheKeys_.end())
      {
        std::string uri = *it;
        orderedCacheKeys_.erase(it);
        orderedCacheKeys_.push_front(uri);
      }

      // create a command and "post" it to the Oracle so it is executed and commited "later"
      NotifyHttpSuccessLater(cache_[uri], payload, successCallback);
    }

  }



  void BaseWebService::NotifyHttpSuccess(const IWebService::HttpRequestSuccessMessage& message)
  {
    if (message.HasPayload())
    {
      dynamic_cast<const BaseWebServicePayload&>(message.GetPayload()).HandleSuccess(message);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }

  void BaseWebService::CacheAndNotifyHttpSuccess(const IWebService::HttpRequestSuccessMessage& message)
  {
    while (cacheCurrentSize_ + message.GetAnswerSize() > cacheMaxSize_ && orderedCacheKeys_.size() > 0)
    {
      const std::string& oldestUri = orderedCacheKeys_.back();
      HttpCache::iterator it = cache_.find(oldestUri);
      if (it != cache_.end())
      {
        cacheCurrentSize_ -= it->second->GetAnswerSize();
        cache_.erase(it);
      }
      orderedCacheKeys_.pop_back();

    }

    cache_[message.GetUri()] = boost::shared_ptr<CachedHttpRequestSuccessMessage>(new CachedHttpRequestSuccessMessage(message));
    orderedCacheKeys_.push_front(message.GetUri());
    cacheCurrentSize_ += message.GetAnswerSize();

    NotifyHttpSuccess(message);
  }

  void BaseWebService::NotifyHttpError(const IWebService::HttpRequestErrorMessage& message)
  {
    if (message.HasPayload())
    {
      dynamic_cast<const BaseWebServicePayload&>(message.GetPayload()).HandleFailure(message);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }




}
