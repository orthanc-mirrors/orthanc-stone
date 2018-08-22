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

#include <Core/IDynamicObject.h>
#include "../../Framework/Messages/IObserver.h"
#include <string>
#include <Core/Logging.h>

namespace OrthancStone
{
  class IWebService
  {
  protected:
    MessageBroker& broker_;
  public:
    typedef std::map<std::string, std::string> Headers;

    class ICallback : public IObserver
    {
    public:
      struct HttpRequestSuccessMessage: public IMessage
      {
        const std::string& Uri;
        const void* Answer;
        size_t AnswerSize;
        Orthanc::IDynamicObject* Payload;
        HttpRequestSuccessMessage(const std::string& uri,
                                  const void* answer,
                                  size_t answerSize,
                                  Orthanc::IDynamicObject* payload)
          : IMessage(MessageType_HttpRequestSuccess),
            Uri(uri),
            Answer(answer),
            AnswerSize(answerSize),
            Payload(payload)
        {}
      };

      struct HttpRequestErrorMessage: public IMessage
      {
        const std::string& Uri;
        Orthanc::IDynamicObject* Payload;
        HttpRequestErrorMessage(const std::string& uri,
                                Orthanc::IDynamicObject* payload)
          : IMessage(MessageType_HttpRequestError),
            Uri(uri),
            Payload(payload)
        {}
      };

      ICallback(MessageBroker& broker)
        : IObserver(broker)
      {
        DeclareHandledMessage(MessageType_HttpRequestError);
        DeclareHandledMessage(MessageType_HttpRequestSuccess);
      }
      virtual ~ICallback()
      {
      }

      virtual void HandleMessage(IObservable& from, const IMessage& message)
      {
        switch(message.GetType())
        {
        case MessageType_HttpRequestError:
        {
          const HttpRequestErrorMessage& msg = dynamic_cast<const HttpRequestErrorMessage&>(message);
          OnHttpRequestError(msg.Uri,
                             msg.Payload);
        }; break;

        case MessageType_HttpRequestSuccess:
        {
          const HttpRequestSuccessMessage& msg = dynamic_cast<const HttpRequestSuccessMessage&>(message);
          OnHttpRequestSuccess(msg.Uri,
                               msg.Answer,
                               msg.AnswerSize,
                               msg.Payload);
        }; break;
        default:
          VLOG("unhandled message type" << message.GetType());
        }
      }

      virtual void OnHttpRequestError(const std::string& uri,
                                      Orthanc::IDynamicObject* payload) = 0;

      virtual void OnHttpRequestSuccess(const std::string& uri,
                                        const void* answer,
                                        size_t answerSize,
                                        Orthanc::IDynamicObject* payload) = 0;
    };

    IWebService(MessageBroker& broker)
      : broker_(broker)
    {}

    virtual ~IWebService()
    {
    }

    virtual void ScheduleGetRequest(ICallback& callback,
                                    const std::string& uri,
                                    const Headers& headers,
                                    Orthanc::IDynamicObject* payload) = 0;

    virtual void SchedulePostRequest(ICallback& callback,
                                     const std::string& uri,
                                     const Headers& headers,
                                     const std::string& body,
                                     Orthanc::IDynamicObject* payload) = 0;
  };
}
