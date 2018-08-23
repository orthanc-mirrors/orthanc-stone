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
#include <Core/OrthancException.h>

namespace OrthancStone {

  struct OrthancApiClient::InternalGetJsonResponseReadyMessage :
      public IMessage
  {
    OrthancApiClient::BaseRequest*  request_;
    Json::Value   response_;

    InternalGetJsonResponseReadyMessage(OrthancApiClient::BaseRequest*  request,
                             const Json::Value& response)
      : IMessage(MessageType_OrthancApi_InternalGetJsonResponseReady),
        request_(request),
        response_(response)
    {
    }

  };

  struct OrthancApiClient::InternalGetJsonResponseErrorMessage :
      public IMessage
  {
    OrthancApiClient::BaseRequest*  request_;

    InternalGetJsonResponseErrorMessage(OrthancApiClient::BaseRequest* request)
      : IMessage(MessageType_OrthancApi_InternalGetJsonResponseError),
        request_(request)
    {
    }
  };


  // this class handles a single request to the OrthancApiClient.
  // Once the response is ready, it will emit a message to the responseObserver
  // the responseObserver must handle only that message (and not all messages from the OrthancApiClient)
  class OrthancApiClient::BaseRequest:
      public IObserver,
      public IObservable,
      public Orthanc::IDynamicObject
  {
  public:
    std::string               uri_;
    OrthancApiClient&         orthanc_;
    MessageType               messageToEmitWhenResponseReady_;
    OrthancApiClient::Mode    mode_;

  public:
    BaseRequest(
        OrthancApiClient& orthanc,
                   IObserver& responseObserver,
                   const std::string& uri,
                   MessageType messageToEmitWhenResponseReady,
                   OrthancApiClient::Mode mode)
      : IObserver(orthanc.broker_),
        IObservable(orthanc.broker_),
        uri_(uri),
        orthanc_(orthanc),
        messageToEmitWhenResponseReady_(messageToEmitWhenResponseReady),
        mode_(mode)
    {
      // this object will emit only a single message, the one the final responseObserver is expecting
      DeclareEmittableMessage(messageToEmitWhenResponseReady);

      // this object is observing the OrthancApi so it must handle all messages
      DeclareHandledMessage(MessageType_OrthancApi_InternalGetJsonResponseReady);
      DeclareIgnoredMessage(MessageType_OrthancApi_InternalGetJsonResponseError);

      orthanc_.RegisterObserver(*this);
      this->RegisterObserver(responseObserver);
    }
    virtual ~BaseRequest() {}

    // mainly maps OrthancApi internal messages to a message that is expected by the responseObserver
    virtual void HandleMessage(IObservable& from, const IMessage& message)
    {
      switch (message.GetType())
      {
        case MessageType_OrthancApi_InternalGetJsonResponseReady:
      {
        const OrthancApiClient::InternalGetJsonResponseReadyMessage& messageReceived = dynamic_cast<const OrthancApiClient::InternalGetJsonResponseReadyMessage&>(message);
        EmitMessage(OrthancApiClient::GetJsonResponseReadyMessage(messageToEmitWhenResponseReady_, messageReceived.request_->uri_, messageReceived.response_));
        orthanc_.ReleaseRequest(messageReceived.request_);
      }; break;
      default:
        throw MessageNotDeclaredException(message.GetType());
      }
    }

  };


  class OrthancApiClient::WebCallback : public IWebService::ICallback
  {
  private:
    OrthancApiClient&  that_;

  public:
    WebCallback(MessageBroker& broker, OrthancApiClient&  that) :
      IWebService::ICallback(broker),
      that_(that)
    {
    }

    virtual void OnHttpRequestSuccess(const std::string& uri,
                                      const void* answer,
                                      size_t answerSize,
                                      Orthanc::IDynamicObject* payload)
    {
      OrthancApiClient::BaseRequest* request = dynamic_cast<OrthancApiClient::BaseRequest*>(payload);  // the BaseRequests objects belongs to the OrthancApiClient and is deleted in ReleaseRequest when it has been "consumed"

      switch (request->mode_)
      {
      case OrthancApiClient::Mode_GetJson:
      {
        Json::Value response;
        if (MessagingToolbox::ParseJson(response, answer, answerSize))
        {
          OrthancApiClient::InternalGetJsonResponseReadyMessage msg(request, response);
          that_.EmitMessage(msg);
        }
        else
        {
          OrthancApiClient::InternalGetJsonResponseErrorMessage msg(request);
          that_.EmitMessage(msg);
        }
      };  break;

      default:
        that_.ReleaseRequest(request);
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }

    virtual void OnHttpRequestError(const std::string& uri,
                                    Orthanc::IDynamicObject* payload)
    {
      OrthancApiClient::BaseRequest* request = dynamic_cast<OrthancApiClient::BaseRequest*>(payload);  // the BaseRequests objects belongs to the OrthancApiClient and is deleted in ReleaseRequest when it has been "consumed"

      switch (request->mode_)
      {
      case OrthancApiClient::Mode_GetJson:
      {
        OrthancApiClient::InternalGetJsonResponseErrorMessage msg(request);
        that_.EmitMessage(msg);
      };  break;

      default:
        that_.ReleaseRequest(request);
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
    }
  };

  OrthancApiClient::OrthancApiClient(MessageBroker &broker, IWebService &orthanc)
    : IObservable(broker),
      orthanc_(orthanc),
      webCallback_(new OrthancApiClient::WebCallback(broker, *this))
  {
    DeclareEmittableMessage(MessageType_OrthancApi_InternalGetJsonResponseReady);
    DeclareEmittableMessage(MessageType_OrthancApi_InternalGetJsonResponseError);
  }

  void OrthancApiClient::ScheduleGetJsonRequest(IObserver &responseObserver, const std::string &uri, MessageType messageToEmitWhenResponseReady)
  {
    OrthancApiClient::BaseRequest* request = new OrthancApiClient::BaseRequest(*this,
                                                                               responseObserver,
                                                                               uri,
                                                                               messageToEmitWhenResponseReady,
                                                                               OrthancApiClient::Mode_GetJson);
    orthanc_.ScheduleGetRequest(*webCallback_, uri, IWebService::Headers(), request);
    requestsInProgress_.insert(request);
  }

  void OrthancApiClient::ReleaseRequest(BaseRequest* request)
  {
    requestsInProgress_.erase(request);
    delete request;
  }

}
