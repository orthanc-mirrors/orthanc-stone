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

#include "OrthancApiClient.h"

#include "../../Toolbox/MessagingToolbox.h"

#include <Core/OrthancException.h>

namespace Deprecated
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
    std::auto_ptr< OrthancStone::MessageHandler<EmptyResponseReadyMessage> >             emptyHandler_;
    std::auto_ptr< OrthancStone::MessageHandler<JsonResponseReadyMessage> >              jsonHandler_;
    std::auto_ptr< OrthancStone::MessageHandler<BinaryResponseReadyMessage> >            binaryHandler_;
    std::auto_ptr< OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage> >  failureHandler_;
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
    WebServicePayload(OrthancStone::MessageHandler<EmptyResponseReadyMessage>* handler,
                      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureHandler,
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

    WebServicePayload(OrthancStone::MessageHandler<BinaryResponseReadyMessage>* handler,
                      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureHandler,
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

    WebServicePayload(OrthancStone::MessageHandler<JsonResponseReadyMessage>* handler,
                      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureHandler,
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
        if (OrthancStone::MessagingToolbox::ParseJson(response, message.GetAnswer(), message.GetAnswerSize()))
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


  OrthancApiClient::OrthancApiClient(OrthancStone::MessageBroker& broker,
                                     IWebService& web,
                                     const std::string& baseUrl) :
    IObservable(broker),
    IObserver(broker),
    web_(web),
    baseUrl_(baseUrl)
  {
  }


  void OrthancApiClient::GetJsonAsync(
      const std::string& uri,
      OrthancStone::MessageHandler<JsonResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    IWebService::HttpHeaders emptyHeaders;
    web_.GetAsync(baseUrl_ + uri,
                  emptyHeaders,
                  new WebServicePayload(successCallback, failureCallback, payload),
                  new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                  (*this, &OrthancApiClient::NotifyHttpSuccess),
                  new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                  (*this, &OrthancApiClient::NotifyHttpError));
  }


  void OrthancApiClient::GetBinaryAsync(
      const std::string& uri,
      const std::string& contentType,
      OrthancStone::MessageHandler<BinaryResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    IWebService::HttpHeaders headers;
    headers["Accept"] = contentType;
    GetBinaryAsync(uri, headers, successCallback, failureCallback, payload);
  }

  void OrthancApiClient::GetBinaryAsync(
      const std::string& uri,
      const IWebService::HttpHeaders& headers,
      OrthancStone::MessageHandler<BinaryResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    // printf("GET [%s] [%s]\n", baseUrl_.c_str(), uri.c_str());

    web_.GetAsync(baseUrl_ + uri, headers,
                  new WebServicePayload(successCallback, failureCallback, payload),
                  new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                  (*this, &OrthancApiClient::NotifyHttpSuccess),
                  new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                  (*this, &OrthancApiClient::NotifyHttpError));
  }

  
  void OrthancApiClient::PostBinaryAsyncExpectJson(
      const std::string& uri,
      const std::string& body,
      OrthancStone::MessageHandler<JsonResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    web_.PostAsync(baseUrl_ + uri, IWebService::HttpHeaders(), body,
                   new WebServicePayload(successCallback, failureCallback, payload),
                   new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                   (*this, &OrthancApiClient::NotifyHttpSuccess),
                   new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                   (*this, &OrthancApiClient::NotifyHttpError));

  }

  void OrthancApiClient::PostBinaryAsync(
      const std::string& uri,
      const std::string& body)
  {
    web_.PostAsync(baseUrl_ + uri, IWebService::HttpHeaders(), body, NULL, NULL, NULL);
  }

  void OrthancApiClient::PostBinaryAsync(
      const std::string& uri,
      const std::string& body,
      OrthancStone::MessageHandler<EmptyResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload   /* takes ownership */)
  {
    web_.PostAsync(baseUrl_ + uri, IWebService::HttpHeaders(), body,
                   new WebServicePayload(successCallback, failureCallback, payload),
                   new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                   (*this, &OrthancApiClient::NotifyHttpSuccess),
                   new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                   (*this, &OrthancApiClient::NotifyHttpError));
  }

  void OrthancApiClient::PostJsonAsyncExpectJson(
      const std::string& uri,
      const Json::Value& data,
      OrthancStone::MessageHandler<JsonResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    std::string body;
    OrthancStone::MessagingToolbox::JsonToString(body, data);
    return PostBinaryAsyncExpectJson(uri, body, successCallback, failureCallback, payload);
  }

  void OrthancApiClient::PostJsonAsync(
      const std::string& uri,
      const Json::Value& data)
  {
    std::string body;
    OrthancStone::MessagingToolbox::JsonToString(body, data);
    return PostBinaryAsync(uri, body);
  }

  void OrthancApiClient::PostJsonAsync(
      const std::string& uri,
      const Json::Value& data,
      OrthancStone::MessageHandler<EmptyResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload   /* takes ownership */)
  {
    std::string body;
    OrthancStone::MessagingToolbox::JsonToString(body, data);
    return PostBinaryAsync(uri, body, successCallback, failureCallback, payload);
  }

  void OrthancApiClient::DeleteAsync(
      const std::string& uri,
      OrthancStone::MessageHandler<EmptyResponseReadyMessage>* successCallback,
      OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    web_.DeleteAsync(baseUrl_ + uri, IWebService::HttpHeaders(),
                     new WebServicePayload(successCallback, failureCallback, payload),
                     new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                     (*this, &OrthancApiClient::NotifyHttpSuccess),
                     new OrthancStone::Callable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
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