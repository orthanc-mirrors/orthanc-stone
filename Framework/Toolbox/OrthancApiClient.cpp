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
      //      public IObserver,
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
      :
        //IObserver(orthanc.broker_),
        IObservable(orthanc.broker_),
        uri_(uri),
        orthanc_(orthanc),
        messageToEmitWhenResponseReady_(messageToEmitWhenResponseReady),
        mode_(mode)
    {
      // this object will emit only a single message, the one the final responseObserver is expecting
      DeclareEmittableMessage(messageToEmitWhenResponseReady);

      //      // this object is observing the OrthancApi so it must handle all messages
      //      DeclareHandledMessage(MessageType_OrthancApi_InternalGetJsonResponseReady);
      //      DeclareIgnoredMessage(MessageType_OrthancApi_InternalGetJsonResponseError);

      //orthanc_.RegisterObserver(*this);
      //this->RegisterObserver(responseObserver);
    }
    virtual ~BaseRequest() {}

    //    // mainly maps OrthancApi internal messages to a message that is expected by the responseObserver
    //    virtual void HandleMessage(IObservable& from, const IMessage& message)
    //    {
    //      switch (message.GetType())
    //      {
    //        case MessageType_OrthancApi_InternalGetJsonResponseReady:
    //      {
    //        const OrthancApiClient::InternalGetJsonResponseReadyMessage& messageReceived = dynamic_cast<const OrthancApiClient::InternalGetJsonResponseReadyMessage&>(message);
    //        EmitMessage(OrthancApiClient::GetJsonResponseReadyMessage(messageToEmitWhenResponseReady_, messageReceived.request_->uri_, messageReceived.response_));
    //        orthanc_.ReleaseRequest(messageReceived.request_);
    //      }; break;
    //      default:
    //        throw MessageNotDeclaredException(message.GetType());
    //      }
    //    }

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
          request->EmitMessage(OrthancApiClient::GetJsonResponseReadyMessage(request->messageToEmitWhenResponseReady_, request->uri_, response));
        }
        else
        {
          //          OrthancApiClient::InternalGetJsonResponseErrorMessage msg(request);
          //          that_.EmitMessage(msg);
        }
      };  break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      that_.ReleaseRequest(request);
    }

    virtual void OnHttpRequestError(const std::string& uri,
                                    Orthanc::IDynamicObject* payload)
    {
      OrthancApiClient::BaseRequest* request = dynamic_cast<OrthancApiClient::BaseRequest*>(payload);  // the BaseRequests objects belongs to the OrthancApiClient and is deleted in ReleaseRequest when it has been "consumed"

      switch (request->mode_)
      {
      case OrthancApiClient::Mode_GetJson:
      {
        //        OrthancApiClient::InternalGetJsonResponseErrorMessage msg(request);
        //        that_.EmitMessage(msg);
        // TODO: the request shall send an error message
      };  break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      that_.ReleaseRequest(request);
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

  // performs the translation between IWebService messages and OrthancApiClient messages
  // TODO: handle destruction of this object (with shared_ptr ?::delete_later ???)
  class HttpResponseToJsonConverter : public IObserver, IObservable
  {
    std::auto_ptr<MessageHandler<OrthancApiClient::NewGetJsonResponseReadyMessage>> orthancApiSuccessCallback_;
    std::auto_ptr<MessageHandler<OrthancApiClient::NewHttpErrorMessage>> orthancApiFailureCallback_;
  public:
    HttpResponseToJsonConverter(MessageBroker& broker,
                                MessageHandler<OrthancApiClient::NewGetJsonResponseReadyMessage>* orthancApiSuccessCallback,
                                MessageHandler<OrthancApiClient::NewHttpErrorMessage>* orthancApiFailureCallback)
      : IObserver(broker),
        IObservable(broker),
        orthancApiSuccessCallback_(orthancApiSuccessCallback),
        orthancApiFailureCallback_(orthancApiFailureCallback)
    {
    }

    void ConvertResponseToJson(const IWebService::NewHttpRequestSuccessMessage& message)
    {
      Json::Value response;
      if (MessagingToolbox::ParseJson(response, message.Answer, message.AnswerSize))
      {
        if (orthancApiSuccessCallback_.get() != NULL)
        {
          orthancApiSuccessCallback_->Apply(OrthancApiClient::NewGetJsonResponseReadyMessage(message.Uri, response));
        }
      }
      else if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(OrthancApiClient::NewHttpErrorMessage(message.Uri));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }

    void ConvertError(const IWebService::NewHttpRequestErrorMessage& message)
    {
      if (orthancApiFailureCallback_.get() != NULL)
      {
        orthancApiFailureCallback_->Apply(OrthancApiClient::NewHttpErrorMessage(message.Uri));
      }

      delete this; // hack untill we find someone to take ownership of this object (https://isocpp.org/wiki/faq/freestore-mgmt#delete-this)
    }
  };

  void OrthancApiClient::GetJsonAsync(const std::string& uri,
                                      MessageHandler<NewGetJsonResponseReadyMessage>* successCallback,
                                      MessageHandler<NewHttpErrorMessage>* failureCallback)
  {
    HttpResponseToJsonConverter* converter = new HttpResponseToJsonConverter(broker_, successCallback, failureCallback);
    orthanc_.GetAsync(uri, IWebService::Headers(), NULL,
                      new Callable<HttpResponseToJsonConverter, IWebService::NewHttpRequestSuccessMessage>(*converter, &HttpResponseToJsonConverter::ConvertResponseToJson),
                      new Callable<HttpResponseToJsonConverter, IWebService::NewHttpRequestErrorMessage>(*converter, &HttpResponseToJsonConverter::ConvertError));

  }

}
