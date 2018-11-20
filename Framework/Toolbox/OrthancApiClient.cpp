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

#include "OrthancApiClient.h"

#include "MessagingToolbox.h"
#include "Framework/Toolbox/MessagingToolbox.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  const Orthanc::IDynamicObject& OrthancApiClient::JsonResponseReadyMessage::GetPayload() const
  {
    if (HasPayload())
    {
      return *payload_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
  
  
  const Orthanc::IDynamicObject& OrthancApiClient::BinaryResponseReadyMessage::GetPayload() const
  {
    if (HasPayload())
    {
      return *payload_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
  
  
  const Orthanc::IDynamicObject& OrthancApiClient::EmptyResponseReadyMessage::GetPayload() const
  {
    if (HasPayload())
    {
      return *payload_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
  
  
  class OrthancApiClient::WebServicePayload : public Orthanc::IDynamicObject
  {
  private:
    std::auto_ptr< MessageHandler<EmptyResponseReadyMessage> >             emptyHandler_;
    std::auto_ptr< MessageHandler<JsonResponseReadyMessage> >              jsonHandler_;
    std::auto_ptr< MessageHandler<BinaryResponseReadyMessage> >            binaryHandler_;
    std::auto_ptr< MessageHandler<IWebService::HttpRequestErrorMessage> >  failureHandler_;
    std::auto_ptr< Orthanc::IDynamicObject >                               userPayload_;

    void NotifyConversionError(const IWebService::HttpRequestSuccessMessage& message) const
    {
      if (failureHandler_.get() != NULL)
      {
        failureHandler_->Apply(IWebService::HttpRequestErrorMessage
                               (message.GetUri(), userPayload_.get()));
      }
    }
    
  public:
    WebServicePayload(MessageHandler<EmptyResponseReadyMessage>* handler,
                      MessageHandler<IWebService::HttpRequestErrorMessage>* failureHandler,
                      Orthanc::IDynamicObject* userPayload) :
      emptyHandler_(handler),
      failureHandler_(failureHandler),
      userPayload_(userPayload)
    {
      if (handler == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    WebServicePayload(MessageHandler<BinaryResponseReadyMessage>* handler,
                      MessageHandler<IWebService::HttpRequestErrorMessage>* failureHandler,
                      Orthanc::IDynamicObject* userPayload) :
      binaryHandler_(handler),
      failureHandler_(failureHandler),
      userPayload_(userPayload)
    {
      if (handler == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    WebServicePayload(MessageHandler<JsonResponseReadyMessage>* handler,
                      MessageHandler<IWebService::HttpRequestErrorMessage>* failureHandler,
                      Orthanc::IDynamicObject* userPayload) :
      jsonHandler_(handler),
      failureHandler_(failureHandler),
      userPayload_(userPayload)
    {
      if (handler == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    void HandleSuccess(const IWebService::HttpRequestSuccessMessage& message) const
    {
      if (emptyHandler_.get() != NULL)
      {
        emptyHandler_->Apply(OrthancApiClient::EmptyResponseReadyMessage
                             (message.GetUri(), userPayload_.get()));
      }
      else if (binaryHandler_.get() != NULL)
      {
        binaryHandler_->Apply(OrthancApiClient::BinaryResponseReadyMessage
                              (message.GetUri(), message.GetAnswer(),
                               message.GetAnswerSize(), userPayload_.get()));
      }
      else if (jsonHandler_.get() != NULL)
      {
        Json::Value response;
        if (MessagingToolbox::ParseJson(response, message.GetAnswer(), message.GetAnswerSize()))
        {
          jsonHandler_->Apply(OrthancApiClient::JsonResponseReadyMessage
                              (message.GetUri(), response, userPayload_.get()));
        }
        else
        {
          NotifyConversionError(message);
        }
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    void HandleFailure(const IWebService::HttpRequestErrorMessage& message) const
    {
      if (failureHandler_.get() != NULL)
      {
        failureHandler_->Apply(IWebService::HttpRequestErrorMessage
                               (message.GetUri(), userPayload_.get()));
      }
    }
  };


  class OrthancApiClient::CachedHttpRequestSuccessMessage
  {
  protected:
    const std::string&             uri_;
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


  OrthancApiClient::OrthancApiClient(MessageBroker& broker,
                                     IWebService& web,
                                     const std::string& baseUrl) :
    IObservable(broker),
    IObserver(broker),
    web_(web),
    baseUrl_(baseUrl),
    cacheEnabled_(true)
  {
  }


  void OrthancApiClient::GetJsonAsync(
      const std::string& uri,
      MessageHandler<JsonResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    IWebService::HttpHeaders emptyHeaders;
    if (cacheEnabled_)
    {
      HandleFromCache(baseUrl_ + uri,
                      emptyHeaders,
                      new WebServicePayload(successCallback, failureCallback, payload));
    }
    else
    {
      web_.GetAsync(baseUrl_ + uri,
                    emptyHeaders,
                    new WebServicePayload(successCallback, failureCallback, payload),
                    new Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                    (*this, &OrthancApiClient::NotifyHttpSuccess),
                    new Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                    (*this, &OrthancApiClient::NotifyHttpError));
    }
  }


  void OrthancApiClient::GetBinaryAsync(
      const std::string& uri,
      const std::string& contentType,
      MessageHandler<BinaryResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    IWebService::HttpHeaders headers;
    headers["Accept"] = contentType;
    GetBinaryAsync(uri, headers, successCallback, failureCallback, payload);
  }
  

  void OrthancApiClient::HandleFromCache(const std::string& uri,
                                         const IWebService::HttpHeaders& headers,
                                         Orthanc::IDynamicObject* payload // takes ownership
                                         )
  {
    if (cache_.find(uri) == cache_.end())
    {
      web_.GetAsync(uri, headers,
                    payload, // ownership is transfered
                    new Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                    (*this, &OrthancApiClient::CacheAndNotifyHttpSuccess),
                    new Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                    (*this, &OrthancApiClient::NotifyHttpError));
    }
    else
    {
      std::auto_ptr<Orthanc::IDynamicObject> payloadRaii(payload); // make sure payload is deleted whatever happens

      const OrthancApiClient::CachedHttpRequestSuccessMessage& cachedMessage = *(cache_[uri]);
      IWebService::HttpRequestSuccessMessage successMessage(cachedMessage.GetUri(),
                                                            cachedMessage.GetAnswer(),
                                                            cachedMessage.GetAnswerSize(),
                                                            cachedMessage.GetAnswerHttpHeaders(),
                                                            payloadRaii.get());
      NotifyHttpSuccess(successMessage);
    }

  }


  void OrthancApiClient::GetBinaryAsync(
      const std::string& uri,
      const IWebService::HttpHeaders& headers,
      MessageHandler<BinaryResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    // printf("GET [%s] [%s]\n", baseUrl_.c_str(), uri.c_str());

    if (cacheEnabled_)
    {
      HandleFromCache(baseUrl_ + uri,
                      headers,
                      new WebServicePayload(successCallback, failureCallback, payload));
    }
    else
    {
      web_.GetAsync(baseUrl_ + uri, headers,
                    new WebServicePayload(successCallback, failureCallback, payload),
                    new Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                    (*this, &OrthancApiClient::NotifyHttpSuccess),
                    new Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                    (*this, &OrthancApiClient::NotifyHttpError));
    }
  }

  
  void OrthancApiClient::PostBinaryAsyncExpectJson(
      const std::string& uri,
      const std::string& body,
      MessageHandler<JsonResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    web_.PostAsync(baseUrl_ + uri, IWebService::HttpHeaders(), body,
                   new WebServicePayload(successCallback, failureCallback, payload),
                   new Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                   (*this, &OrthancApiClient::NotifyHttpSuccess),
                   new Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                   (*this, &OrthancApiClient::NotifyHttpError));

  }

  
  void OrthancApiClient::PostJsonAsyncExpectJson(
      const std::string& uri,
      const Json::Value& data,
      MessageHandler<JsonResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    std::string body;
    MessagingToolbox::JsonToString(body, data);
    return PostBinaryAsyncExpectJson(uri, body, successCallback, failureCallback, payload);
  }

  
  void OrthancApiClient::DeleteAsync(
      const std::string& uri,
      MessageHandler<EmptyResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    web_.DeleteAsync(baseUrl_ + uri, IWebService::HttpHeaders(),
                     new WebServicePayload(successCallback, failureCallback, payload),
                     new Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                     (*this, &OrthancApiClient::NotifyHttpSuccess),
                     new Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                     (*this, &OrthancApiClient::NotifyHttpError));
  }


  void OrthancApiClient::NotifyHttpSuccess(const IWebService::HttpRequestSuccessMessage& message)
  {
    if (message.HasPayload())
    {
      dynamic_cast<const WebServicePayload&>(message.GetPayload()).HandleSuccess(message);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }

  void OrthancApiClient::CacheAndNotifyHttpSuccess(const IWebService::HttpRequestSuccessMessage& message)
  {
    cache_[message.GetUri()] = boost::shared_ptr<CachedHttpRequestSuccessMessage>(new CachedHttpRequestSuccessMessage(message));
    NotifyHttpSuccess(message);
  }

  void OrthancApiClient::NotifyHttpError(const IWebService::HttpRequestErrorMessage& message)
  {
    if (message.HasPayload())
    {
      dynamic_cast<const WebServicePayload&>(message.GetPayload()).HandleFailure(message);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }
}
