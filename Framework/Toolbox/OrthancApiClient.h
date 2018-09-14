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
  class OrthancApiClient:
      public IObservable
  {
  protected:
    class BaseRequest;
    class GetJsonRequest;

    struct InternalGetJsonResponseReadyMessage;
    struct InternalGetJsonResponseErrorMessage;

  public:
    struct GetJsonResponseReadyMessage : public IMessage
    {
      Json::Value   response_;
      std::string   uri_;

      GetJsonResponseReadyMessage(MessageType messageType,
                                  const std::string& uri,
                                  const Json::Value& response)
        : IMessage(messageType),
          response_(response),
          uri_(uri)
      {
      }
    };

    struct NewGetJsonResponseReadyMessage : public BaseMessage<MessageType_OrthancApi_GenericGetJson_Ready>
    {
      Json::Value   response_;
      std::string   uri_;

      NewGetJsonResponseReadyMessage(const std::string& uri,
                                     const Json::Value& response)
        : BaseMessage(),
          response_(response),
          uri_(uri)
      {
      }
    };

    struct NewHttpErrorMessage : public BaseMessage<MessageType_OrthancApi_GenericHttpError_Ready>
    {
      std::string   uri_;

      NewHttpErrorMessage(const std::string& uri)
        : BaseMessage(),
          uri_(uri)
      {
      }
    };


  public:

    enum Mode
    {
      Mode_GetJson
    };

  protected:
    IWebService&                      orthanc_;
    class WebCallback;
    boost::shared_ptr<WebCallback>    webCallback_;  // This is a PImpl pattern
    std::set<BaseRequest*>            requestsInProgress_;

    //    int ScheduleGetJsonRequest(const std::string& uri);

    void ReleaseRequest(BaseRequest* request);

  public:
    OrthancApiClient(MessageBroker& broker,
                     IWebService& orthanc);
    virtual ~OrthancApiClient() {}

    // schedule a GET request expecting a JSON request.
    // once the response is ready, it will emit a OrthancApiClient::GetJsonResponseReadyMessage message whose messageType is specified in the call
    void ScheduleGetJsonRequest(IObserver& responseObserver, const std::string& uri, MessageType messageToEmitWhenResponseReady);

    void ScheduleGetStudyIds(IObserver& responseObserver) {ScheduleGetJsonRequest(responseObserver, "/studies", MessageType_OrthancApi_GetStudyIds_Ready);}
    void ScheduleGetStudy(IObserver& responseObserver, const std::string& studyId) {ScheduleGetJsonRequest(responseObserver, "/studies/" + studyId, MessageType_OrthancApi_GetStudy_Ready);}
    void ScheduleGetSeries(IObserver& responseObserver, const std::string& seriesId) {ScheduleGetJsonRequest(responseObserver, "/series/" + seriesId, MessageType_OrthancApi_GetSeries_Ready);}

    void GetJsonAsync(const std::string& uri,
                      MessageHandler<NewGetJsonResponseReadyMessage>* successCallback,
                      MessageHandler<NewHttpErrorMessage>* failureCallback = NULL);



  };
}
