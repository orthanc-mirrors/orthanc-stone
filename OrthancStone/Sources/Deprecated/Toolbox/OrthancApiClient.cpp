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

#include "OrthancApiClient.h"

#include "../Toolbox/MessagingToolbox.h"

#include <OrthancException.h>

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
    std::unique_ptr< MessageHandler<EmptyResponseReadyMessage> >             emptyHandler_;
    std::unique_ptr< MessageHandler<JsonResponseReadyMessage> >              jsonHandler_;
    std::unique_ptr< MessageHandler<BinaryResponseReadyMessage> >            binaryHandler_;
    std::unique_ptr< MessageHandler<IWebService::HttpRequestErrorMessage> >  failureHandler_;
    std::unique_ptr< Orthanc::IDynamicObject >                               userPayload_;

    void NotifyConversionError(const IWebService::HttpRequestSuccessMessage& message) const
    {
      if (failureHandler_.get() != NULL)
      {
        failureHandler_->Apply(IWebService::HttpRequestErrorMessage
                               (message.GetUri(), Orthanc::HttpStatus_None, userPayload_.get()));
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
                               (message.GetUri(), message.GetHttpStatus(), userPayload_.get()));
      }
    }
  };


  OrthancApiClient::OrthancApiClient(IWebService& web,
                                     const std::string& baseUrl) :
    web_(web),
    baseUrl_(baseUrl)
  {
  }


  void OrthancApiClient::GetJsonAsync(
      const std::string& uri,
      MessageHandler<JsonResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    IWebService::HttpHeaders emptyHeaders;
    web_.GetAsync(baseUrl_ + uri,
                  emptyHeaders,
                  new WebServicePayload(successCallback, failureCallback, payload),
                  new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                  (GetSharedObserver(), &OrthancApiClient::NotifyHttpSuccess),
                  new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                  (GetSharedObserver(), &OrthancApiClient::NotifyHttpError));
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

  void OrthancApiClient::GetBinaryAsync(
      const std::string& uri,
      const IWebService::HttpHeaders& headers,
      MessageHandler<BinaryResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    // printf("GET [%s] [%s]\n", baseUrl_.c_str(), uri.c_str());

    web_.GetAsync(baseUrl_ + uri, headers,
                  new WebServicePayload(successCallback, failureCallback, payload),
                  new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                  (GetSharedObserver(), &OrthancApiClient::NotifyHttpSuccess),
                  new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                  (GetSharedObserver(), &OrthancApiClient::NotifyHttpError));
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
                   new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                   (GetSharedObserver(), &OrthancApiClient::NotifyHttpSuccess),
                   new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                   (GetSharedObserver(), &OrthancApiClient::NotifyHttpError));

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
      MessageHandler<EmptyResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload   /* takes ownership */)
  {
    web_.PostAsync(baseUrl_ + uri, IWebService::HttpHeaders(), body,
                   new WebServicePayload(successCallback, failureCallback, payload),
                   new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                   (GetSharedObserver(), &OrthancApiClient::NotifyHttpSuccess),
                   new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                   (GetSharedObserver(), &OrthancApiClient::NotifyHttpError));
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

  void OrthancApiClient::PostJsonAsync(
      const std::string& uri,
      const Json::Value& data)
  {
    std::string body;
    MessagingToolbox::JsonToString(body, data);
    return PostBinaryAsync(uri, body);
  }

  void OrthancApiClient::PostJsonAsync(
      const std::string& uri,
      const Json::Value& data,
      MessageHandler<EmptyResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload   /* takes ownership */)
  {
    std::string body;
    MessagingToolbox::JsonToString(body, data);
    return PostBinaryAsync(uri, body, successCallback, failureCallback, payload);
  }

  void OrthancApiClient::DeleteAsync(
      const std::string& uri,
      MessageHandler<EmptyResponseReadyMessage>* successCallback,
      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
      Orthanc::IDynamicObject* payload)
  {
    web_.DeleteAsync(baseUrl_ + uri, IWebService::HttpHeaders(),
                     new WebServicePayload(successCallback, failureCallback, payload),
                     new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestSuccessMessage>
                     (GetSharedObserver(), &OrthancApiClient::NotifyHttpSuccess),
                     new DeprecatedCallable<OrthancApiClient, IWebService::HttpRequestErrorMessage>
                     (GetSharedObserver(), &OrthancApiClient::NotifyHttpError));
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
