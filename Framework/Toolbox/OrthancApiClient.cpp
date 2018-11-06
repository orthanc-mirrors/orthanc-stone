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
  
  
  OrthancApiClient::OrthancApiClient(MessageBroker& broker,
                                     IWebService& orthanc) :
    IObservable(broker),
    orthanc_(orthanc)
  {
  }

  // performs the translation between IWebService messages and OrthancApiClient messages
  // TODO: handle destruction of this object (with shared_ptr ?::delete_later ???)
  class HttpResponseToJsonConverter : public IObserver, IObservable
  {
  private:
    std::auto_ptr<MessageHandler<OrthancApiClient::JsonResponseReadyMessage> > orthancApiSuccessCallback_;
    std::auto_ptr<MessageHandler<IWebService::HttpRequestErrorMessage> > orthancApiFailureCallback_;

  public:
    HttpResponseToJsonConverter(MessageBroker& broker,
                                MessageHandler<OrthancApiClient::JsonResponseReadyMessage>* orthancApiSuccessCallback,
                                MessageHandler<IWebService::HttpRequestErrorMessage>* orthancApiFailureCallback)
      : IObserver(broker),
        IObservable(broker),
        orthancApiSuccessCallback_(orthancApiSuccessCallback),
        orthancApiFailureCallback_(orthancApiFailureCallback)
    {
    }

    void ConvertResponseToJson(const IWebService::HttpRequestSuccessMessage& message)
    {
      Json::Value response;
      if (MessagingToolbox::ParseJson(response, message.GetAnswer(), message.GetAnswerSize()))
      {
        if (orthancApiSuccessCallback_.get() != NULL)
        {
          orthancApiSuccessCallback_->Apply(OrthancApiClient::JsonResponseReadyMessage
                                            (message.GetUri(), response, message.GetPayloadPointer()));
        }
      }
      else if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(IWebService::HttpRequestErrorMessage
                                          (message.GetUri(), message.GetPayloadPointer()));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }

    void ConvertError(const IWebService::HttpRequestErrorMessage& message)
    {
      if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(IWebService::HttpRequestErrorMessage(message.GetUri(), message.GetPayloadPointer()));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }
  };

  // performs the translation between IWebService messages and OrthancApiClient messages
  // TODO: handle destruction of this object (with shared_ptr ?::delete_later ???)
  class HttpResponseToBinaryConverter : public IObserver, IObservable
  {
  private:
    std::auto_ptr<MessageHandler<OrthancApiClient::BinaryResponseReadyMessage> > orthancApiSuccessCallback_;
    std::auto_ptr<MessageHandler<IWebService::HttpRequestErrorMessage> > orthancApiFailureCallback_;

  public:
    HttpResponseToBinaryConverter(MessageBroker& broker,
                                  MessageHandler<OrthancApiClient::BinaryResponseReadyMessage>* orthancApiSuccessCallback,
                                  MessageHandler<IWebService::HttpRequestErrorMessage>* orthancApiFailureCallback)
      : IObserver(broker),
        IObservable(broker),
        orthancApiSuccessCallback_(orthancApiSuccessCallback),
        orthancApiFailureCallback_(orthancApiFailureCallback)
    {
    }

    void ConvertResponseToBinary(const IWebService::HttpRequestSuccessMessage& message)
    {
      if (orthancApiSuccessCallback_.get() != NULL)
      {
        orthancApiSuccessCallback_->Apply(OrthancApiClient::BinaryResponseReadyMessage
                                          (message.GetUri(), message.GetAnswer(),
                                           message.GetAnswerSize(), message.GetPayloadPointer()));
      }
      else if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(IWebService::HttpRequestErrorMessage
                                          (message.GetUri(), message.GetPayloadPointer()));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }

    void ConvertError(const IWebService::HttpRequestErrorMessage& message)
    {
      if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(IWebService::HttpRequestErrorMessage(message.GetUri(), message.GetPayloadPointer()));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }
  };

  // performs the translation between IWebService messages and OrthancApiClient messages
  // TODO: handle destruction of this object (with shared_ptr ?::delete_later ???)
  class HttpResponseToEmptyConverter : public IObserver, IObservable
  {
  private:
    std::auto_ptr<MessageHandler<OrthancApiClient::EmptyResponseReadyMessage> > orthancApiSuccessCallback_;
    std::auto_ptr<MessageHandler<IWebService::HttpRequestErrorMessage> > orthancApiFailureCallback_;

  public:
    HttpResponseToEmptyConverter(MessageBroker& broker,
                                  MessageHandler<OrthancApiClient::EmptyResponseReadyMessage>* orthancApiSuccessCallback,
                                  MessageHandler<IWebService::HttpRequestErrorMessage>* orthancApiFailureCallback)
      : IObserver(broker),
        IObservable(broker),
        orthancApiSuccessCallback_(orthancApiSuccessCallback),
        orthancApiFailureCallback_(orthancApiFailureCallback)
    {
    }

    void ConvertResponseToEmpty(const IWebService::HttpRequestSuccessMessage& message)
    {
      if (orthancApiSuccessCallback_.get() != NULL)
      {
        orthancApiSuccessCallback_->Apply(OrthancApiClient::EmptyResponseReadyMessage
                                          (message.GetUri(), message.GetPayloadPointer()));
      }
      else if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(IWebService::HttpRequestErrorMessage
                                          (message.GetUri(), message.GetPayloadPointer()));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }

    void ConvertError(const IWebService::HttpRequestErrorMessage& message)
    {
      if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(IWebService::HttpRequestErrorMessage(message.GetUri(), message.GetPayloadPointer()));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }
  };


  void OrthancApiClient::GetJsonAsync(const std::string& uri,
                                      MessageHandler<JsonResponseReadyMessage>* successCallback,
                                      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                      Orthanc::IDynamicObject* payload)
  {
    HttpResponseToJsonConverter* converter = new HttpResponseToJsonConverter(broker_, successCallback, failureCallback);  // it is currently deleting itself after being used
    orthanc_.GetAsync(uri, IWebService::Headers(), payload,
                      new Callable<HttpResponseToJsonConverter, IWebService::HttpRequestSuccessMessage>(*converter, &HttpResponseToJsonConverter::ConvertResponseToJson),
                      new Callable<HttpResponseToJsonConverter, IWebService::HttpRequestErrorMessage>(*converter, &HttpResponseToJsonConverter::ConvertError));

  }


  void OrthancApiClient::GetBinaryAsync(const std::string& uri,
                                        const std::string& contentType,
                                        MessageHandler<BinaryResponseReadyMessage>* successCallback,
                                        MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                        Orthanc::IDynamicObject* payload)
  {
    IWebService::Headers headers;
    headers["Accept"] = contentType;
    GetBinaryAsync(uri, headers, successCallback, failureCallback, payload);
  }
  

  void OrthancApiClient::GetBinaryAsync(const std::string& uri,
                                        const IWebService::Headers& headers,
                                        MessageHandler<BinaryResponseReadyMessage>* successCallback,
                                        MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                        Orthanc::IDynamicObject* payload)
  {
    HttpResponseToBinaryConverter* converter = new HttpResponseToBinaryConverter(broker_, successCallback, failureCallback);  // it is currently deleting itself after being used
    orthanc_.GetAsync(uri, headers, payload,
                      new Callable<HttpResponseToBinaryConverter, IWebService::HttpRequestSuccessMessage>(*converter, &HttpResponseToBinaryConverter::ConvertResponseToBinary),
                      new Callable<HttpResponseToBinaryConverter, IWebService::HttpRequestErrorMessage>(*converter, &HttpResponseToBinaryConverter::ConvertError));
  }

  
  void OrthancApiClient::PostBinaryAsyncExpectJson(const std::string& uri,
                                                   const std::string& body,
                                                   MessageHandler<JsonResponseReadyMessage>* successCallback,
                                                   MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                                   Orthanc::IDynamicObject* payload)
  {
    HttpResponseToJsonConverter* converter = new HttpResponseToJsonConverter(broker_, successCallback, failureCallback);  // it is currently deleting itself after being used
    orthanc_.PostAsync(uri, IWebService::Headers(), body, payload,
                       new Callable<HttpResponseToJsonConverter, IWebService::HttpRequestSuccessMessage>(*converter, &HttpResponseToJsonConverter::ConvertResponseToJson),
                       new Callable<HttpResponseToJsonConverter, IWebService::HttpRequestErrorMessage>(*converter, &HttpResponseToJsonConverter::ConvertError));

  }

  
  void OrthancApiClient::PostJsonAsyncExpectJson(const std::string& uri,
                                                 const Json::Value& data,
                                                 MessageHandler<JsonResponseReadyMessage>* successCallback,
                                                 MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                                 Orthanc::IDynamicObject* payload)
  {
    std::string body;
    MessagingToolbox::JsonToString(body, data);
    return PostBinaryAsyncExpectJson(uri, body, successCallback, failureCallback, payload);
  }

  
  void OrthancApiClient::DeleteAsync(const std::string& uri,
                                     MessageHandler<EmptyResponseReadyMessage>* successCallback,
                                     MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback,
                                     Orthanc::IDynamicObject* payload)
  {
    HttpResponseToEmptyConverter* converter = new HttpResponseToEmptyConverter(broker_, successCallback, failureCallback);  // it is currently deleting itself after being used
    orthanc_.DeleteAsync(uri, IWebService::Headers(), payload,
                       new Callable<HttpResponseToEmptyConverter, IWebService::HttpRequestSuccessMessage>(*converter, &HttpResponseToEmptyConverter::ConvertResponseToEmpty),
                       new Callable<HttpResponseToEmptyConverter, IWebService::HttpRequestErrorMessage>(*converter, &HttpResponseToEmptyConverter::ConvertError));
  }


  class OrthancApiClient::WebServicePayload : public boost::noncopyable
  {
  private:
    std::auto_ptr< MessageHandler<EmptyResponseReadyMessage> >   emptyHandler_;
    std::auto_ptr< MessageHandler<JsonResponseReadyMessage> >    jsonHandler_;
    std::auto_ptr< MessageHandler<BinaryResponseReadyMessage> >  binaryHandler_;
    std::auto_ptr< Orthanc::IDynamicObject >                     userPayload;
  };
}
