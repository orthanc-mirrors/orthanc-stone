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

#include "../../Framework/Messages/IObserver.h"
#include "../../Framework/Messages/ICallable.h"

#include <Core/IDynamicObject.h>
#include <Core/Logging.h>

#include <string>
#include <map>

namespace OrthancStone
{
  // The IWebService performs HTTP requests.
  // Since applications can run in native or WASM environment and, since
  // in a WASM environment, the WebService is asynchronous, the IWebservice
  // also implements an asynchronous interface: you must schedule a request
  // and you'll be notified when the response/error is ready.
  class IWebService : public boost::noncopyable
  {
  protected:
    MessageBroker& broker_;
    
  public:
    typedef std::map<std::string, std::string> Headers;

    class HttpRequestSuccessMessage : public BaseMessage<MessageType_HttpRequestSuccess>
    {
    private:
      const std::string&             uri_;
      const void*                    answer_;
      size_t                         answerSize_;
      const Orthanc::IDynamicObject* payload_;

    public:
      HttpRequestSuccessMessage(const std::string& uri,
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

      const Orthanc::IDynamicObject* GetPayloadPointer() const
      {
        return payload_;
      }
    };
    

    class HttpRequestErrorMessage : public BaseMessage<MessageType_HttpRequestError>
    {
    private:
      const std::string&              uri_;
      const Orthanc::IDynamicObject*  payload_;

    public:
      HttpRequestErrorMessage(const std::string& uri,
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

      const Orthanc::IDynamicObject* GetPayloadPointer() const
      {
        return payload_;
      }
    };


    IWebService(MessageBroker& broker) :
      broker_(broker)
    {
    }

    
    virtual ~IWebService()
    {
    }

    
    virtual void GetAsync(const std::string& uri,
                          const Headers& headers,
                          Orthanc::IDynamicObject* payload  /* takes ownership */,
                          MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                          MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                          unsigned int timeoutInSeconds = 60) = 0;

    virtual void PostAsync(const std::string& uri,
                           const Headers& headers,
                           const std::string& body,
                           Orthanc::IDynamicObject* payload  /* takes ownership */,
                           MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                           MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                           unsigned int timeoutInSeconds = 60) = 0;

    virtual void DeleteAsync(const std::string& uri,
                             const Headers& headers,
                             Orthanc::IDynamicObject* payload  /* takes ownership */,
                             MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                             MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                             unsigned int timeoutInSeconds = 60) = 0;
  };
}
