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


#pragma once

#include <boost/shared_ptr.hpp>
#include <json/json.h>

#include "IWebService.h"
#include "../../Messages/ObserverBase.h"

namespace Deprecated
{
  enum SliceImageQuality
  {
    SliceImageQuality_FullPng,  // smaller to transmit but longer to generate on Orthanc side (better choice when on low bandwidth)
    SliceImageQuality_FullPam,  // bigger to transmit but faster to generate on Orthanc side (better choice when on localhost or LAN)
    SliceImageQuality_Jpeg50,
    SliceImageQuality_Jpeg90,
    SliceImageQuality_Jpeg95,

    SliceImageQuality_InternalRaw   // downloads the raw pixels data as they are stored in the DICOM file (internal use only)
  };  

  class OrthancApiClient :
      public OrthancStone::IObservable,
      public OrthancStone::ObserverBase<OrthancApiClient>
  {
  public:
    class JsonResponseReadyMessage : public OrthancStone::IMessage
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

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
    

    class BinaryResponseReadyMessage : public OrthancStone::IMessage
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

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


    class EmptyResponseReadyMessage : public OrthancStone::IMessage
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);

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

    

  private:
    class WebServicePayload;

  protected:
    IWebService&  web_;
    std::string   baseUrl_;

  public:
    OrthancApiClient(IWebService& web,
                     const std::string& baseUrl);
    
    virtual ~OrthancApiClient()
    {
    }

    const std::string& GetBaseUrl() const {return baseUrl_;}

    // schedule a GET request expecting a JSON response.
    void GetJsonAsync(const std::string& uri,
                      MessageHandler<JsonResponseReadyMessage>* successCallback,
                      MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                      Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a GET request expecting a binary response.
    void GetBinaryAsync(const std::string& uri,
                        const std::string& contentType,
                        MessageHandler<BinaryResponseReadyMessage>* successCallback,
                        MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                        Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a GET request expecting a binary response.
    void GetBinaryAsync(const std::string& uri,
                        const IWebService::HttpHeaders& headers,
                        MessageHandler<BinaryResponseReadyMessage>* successCallback,
                        MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                        Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a POST request expecting a JSON response.
    void PostBinaryAsyncExpectJson(const std::string& uri,
                                   const std::string& body,
                                   MessageHandler<JsonResponseReadyMessage>* successCallback,
                                   MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                                   Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a POST request expecting a JSON response.
    void PostJsonAsyncExpectJson(const std::string& uri,
                                 const Json::Value& data,
                                 MessageHandler<JsonResponseReadyMessage>* successCallback,
                                 MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                                 Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a POST request and don't mind the response.
    void PostJsonAsync(const std::string& uri,
                       const Json::Value& data);

    // schedule a POST request and don't expect any response.
    void PostJsonAsync(const std::string& uri,
                       const Json::Value& data,
                       MessageHandler<EmptyResponseReadyMessage>* successCallback,
                       MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                       Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);


    // schedule a POST request and don't mind the response.
    void PostBinaryAsync(const std::string& uri,
                         const std::string& body);

    // schedule a POST request and don't expect any response.
    void PostBinaryAsync(const std::string& uri,
                         const std::string& body,
                         MessageHandler<EmptyResponseReadyMessage>* successCallback,
                         MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                         Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    // schedule a DELETE request expecting an empty response.
    void DeleteAsync(const std::string& uri,
                     MessageHandler<EmptyResponseReadyMessage>* successCallback,
                     MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallback = NULL,
                     Orthanc::IDynamicObject* payload = NULL   /* takes ownership */);

    void NotifyHttpSuccess(const IWebService::HttpRequestSuccessMessage& message);

    void NotifyHttpError(const IWebService::HttpRequestErrorMessage& message);

  private:
    void HandleFromCache(const std::string& uri,
                         const IWebService::HttpHeaders& headers,
                         Orthanc::IDynamicObject* payload /* takes ownership */);
  };
}
