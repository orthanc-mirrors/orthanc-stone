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


#pragma once

#include "../../Messages/IObserver.h"
#include "../../Messages/ICallable.h"

#include <Core/Enumerations.h>
#include <Core/IDynamicObject.h>
#include <Core/Logging.h>

#include <string>
#include <map>

namespace Deprecated
{
  template <typename TMessage>
  class MessageHandler : public OrthancStone::ICallable
  {
  };


  template <typename TObserver,
            typename TMessage>
  class DeprecatedCallable : public MessageHandler<TMessage>
  {
  private:
    typedef void (TObserver::* MemberMethod) (const TMessage&);

    boost::weak_ptr<OrthancStone::IObserver>  observer_;
    MemberMethod                function_;

  public:
    DeprecatedCallable(boost::shared_ptr<TObserver> observer,
                       MemberMethod function) :
      observer_(observer),
      function_(function)
    {
    }

    virtual void Apply(const OrthancStone::IMessage& message)
    {
      boost::shared_ptr<OrthancStone::IObserver> lock(observer_);
      if (lock)
      {
        TObserver& observer = dynamic_cast<TObserver&>(*lock);
        const TMessage& typedMessage = dynamic_cast<const TMessage&>(message);
        (observer.*function_) (typedMessage);
      }
    }

    virtual const OrthancStone::MessageIdentifier& GetMessageIdentifier()
    {
      return TMessage::GetStaticIdentifier();
    }

    virtual boost::weak_ptr<OrthancStone::IObserver> GetObserver() const
    {
      return observer_;
    }
  };


  // The IWebService performs HTTP requests.
  // Since applications can run in native or WASM environment and, since
  // in a WASM environment, the WebService is asynchronous, the IWebservice
  // also implements an asynchronous interface: you must schedule a request
  // and you'll be notified when the response/error is ready.
  class IWebService : public boost::noncopyable
  {
  public:
    typedef std::map<std::string, std::string> HttpHeaders;

    class HttpRequestSuccessMessage : public OrthancStone::IMessage
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

    private:
      const std::string&             uri_;
      const void*                    answer_;
      size_t                         answerSize_;
      const HttpHeaders&             answerHeaders_;
      const Orthanc::IDynamicObject* payload_;

    public:
      HttpRequestSuccessMessage(const std::string& uri,
                                const void* answer,
                                size_t answerSize,
                                const HttpHeaders& answerHeaders,
                                const Orthanc::IDynamicObject* payload) :
        uri_(uri),
        answer_(answer),
        answerSize_(answerSize),
        answerHeaders_(answerHeaders),
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

      const HttpHeaders&  GetAnswerHttpHeaders() const
      {
        return answerHeaders_;
      }

      bool HasPayload() const
      {
        return payload_ != NULL;
      }

      const Orthanc::IDynamicObject& GetPayload() const;
    };
    

    class HttpRequestErrorMessage : public OrthancStone::IMessage
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

    private:
      const std::string&              uri_;
      const Orthanc::IDynamicObject*  payload_;
      Orthanc::HttpStatus             httpStatus_;

    public:
      HttpRequestErrorMessage(const std::string& uri,
                              Orthanc::HttpStatus httpStatus,
                              const Orthanc::IDynamicObject* payload) :
        uri_(uri),
        payload_(payload),
        httpStatus_(httpStatus)
      {
      }

      const std::string& GetUri() const
      {
        return uri_;
      }

      Orthanc::HttpStatus GetHttpStatus() const
      {
        return httpStatus_;
      }

      bool HasPayload() const
      {
        return payload_ != NULL;
      }

      const Orthanc::IDynamicObject& GetPayload() const;
    };


    virtual ~IWebService()
    {
    }

    virtual void EnableCache(bool enable) = 0;
    
    virtual void GetAsync(const std::string& uri,
                          const HttpHeaders& headers,
                          Orthanc::IDynamicObject* payload  /* takes ownership */,
                          MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                          MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                          unsigned int timeoutInSeconds = 60) = 0;

    virtual void PostAsync(const std::string& uri,
                           const HttpHeaders& headers,
                           const std::string& body,
                           Orthanc::IDynamicObject* payload  /* takes ownership */,
                           MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                           MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                           unsigned int timeoutInSeconds = 60) = 0;

    virtual void DeleteAsync(const std::string& uri,
                             const HttpHeaders& headers,
                             Orthanc::IDynamicObject* payload  /* takes ownership */,
                             MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback,
                             MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                             unsigned int timeoutInSeconds = 60) = 0;
  };
}
