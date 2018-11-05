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

#include <boost/shared_ptr.hpp>
#include <json/json.h>

#include "IWebService.h"
#include "../Messages/IObservable.h"
#include "../Messages/Promise.h"

namespace OrthancStone
{
  class OrthancApiClient : public IObservable
  {
  public:
    class JsonResponseReadyMessage : public BaseMessage<MessageType_OrthancApi_GenericGetJson_Ready>
    {
    private:
      const std::string&              uri_;
      const Json::Value&              json_;
      const Orthanc::IDynamicObject*  payload_;

    public:
      JsonResponseReadyMessage(const std::string& uri,
                               const Json::Value& json,
                               const Orthanc::IDynamicObject* payload) :
        uri_(uri),
        json_(json),
        payload_(payload)
      {
      }

      const std::string& GetUri() const
      {
        return uri_;
      }

      const Json::Value& GetJson() const
      {
        return json_;
      }

      bool HasPayload() const
      {
        return payload_ != NULL;
      }

      const Orthanc::IDynamicObject& GetPayload() const;
    };
    

    class BinaryResponseReadyMessage : public BaseMessage<MessageType_OrthancApi_GenericGetBinary_Ready>
    {
    private:
      const std::string&              uri_;
      const void*                     answer_;
      size_t                          answerSize_;
      const Orthanc::IDynamicObject*  payload_;

    public:
      BinaryResponseReadyMessage(const std::string& uri,
                                 const void* answer,
                                 size_t answerSize,
                                 const Orthanc::IDynamicObject* payload) :
        uri_(uri),
        answer_(answer),
        answerSize_(answerSize),
        payload_(payload)
      {
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

      bool HasPayload() const
      {
        return payload_ != NULL;
      }

      const Orthanc::IDynamicObject& GetPayload() const;
    };


    class EmptyResponseReadyMessage : public BaseMessage<MessageType_OrthancApi_GenericEmptyResponse_Ready>
    {
    private:
      const std::string&              uri_;
      const Orthanc::IDynamicObject*  payload_;

    public:
      EmptyResponseReadyMessage(const std::string& uri,
                                const Orthanc::IDynamicObject* payload) :
        uri_(uri),
        payload_(payload)
      {
      }

      const std::string& GetUri() const
      {
        return uri_;
      }

      bool HasPayload() const
      {
        return payload_ != NULL;
      }

      const Orthanc::IDynamicObject& GetPayload() const;
    };

    
    class HttpErrorMessage : public BaseMessage<MessageType_OrthancApi_GenericHttpError_Ready>
    {
    private:
      const std::string&              uri_;
      const Orthanc::IDynamicObject*  payload_;

    public:
      HttpErrorMessage(const std::string& uri,
                       const Orthanc::IDynamicObject* payload) :
        uri_(uri),
        payload_(payload)
      {
      }

      const std::string& GetUri() const
      {
        return uri_;
      }

      bool HasPayload() const
      {
        return payload_ != NULL;
      }

      const Orthanc::IDynamicObject& GetPayload() const;
    };


  protected:
    IWebService&                      orthanc_;

  public:
    OrthancApiClient(MessageBroker& broker,
                     IWebService& orthanc);
    
    virtual ~OrthancApiClient()
    {
    }

    // schedule a GET request expecting a JSON response.
    void GetJsonAsync(const std::string& uri,
                      MessageHandler<JsonResponseReadyMessage>* successCallback,
                      MessageHandler<HttpErrorMessage>* failureCallback = NULL,
                      Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a GET request expecting a binary response.
    void GetBinaryAsync(const std::string& uri,
                        const std::string& contentType,
                        MessageHandler<BinaryResponseReadyMessage>* successCallback,
                        MessageHandler<HttpErrorMessage>* failureCallback = NULL,
                        Orthanc::IDynamicObject* payload = NULL   /* takes ownership */)
    {
      IWebService::Headers headers;
      headers["Accept"] = contentType;
      GetBinaryAsync(uri, headers, successCallback, failureCallback, payload);
    }

    // schedule a GET request expecting a binary response.
    void GetBinaryAsync(const std::string& uri,
                        const IWebService::Headers& headers,
                        MessageHandler<BinaryResponseReadyMessage>* successCallback,
                        MessageHandler<HttpErrorMessage>* failureCallback = NULL,
                        Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a POST request expecting a JSON response.
    void PostBinaryAsyncExpectJson(const std::string& uri,
                                   const std::string& body,
                                   MessageHandler<JsonResponseReadyMessage>* successCallback,
                                   MessageHandler<HttpErrorMessage>* failureCallback = NULL,
                                   Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a POST request expecting a JSON response.
    void PostJsonAsyncExpectJson(const std::string& uri,
                                 const Json::Value& data,
                                 MessageHandler<JsonResponseReadyMessage>* successCallback,
                                 MessageHandler<HttpErrorMessage>* failureCallback = NULL,
                                 Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a DELETE request expecting an empty response.
    void DeleteAsync(const std::string& uri,
                     MessageHandler<EmptyResponseReadyMessage>* successCallback,
                     MessageHandler<HttpErrorMessage>* failureCallback = NULL,
                     Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);


  };
}
