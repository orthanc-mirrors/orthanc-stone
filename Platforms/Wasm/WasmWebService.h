#pragma once

#include <Framework/Toolbox/IWebService.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  class WasmWebService : public IWebService
  {
  private:
    static MessageBroker* broker_;

    // Private constructor => Singleton design pattern
    WasmWebService(MessageBroker& broker) :
      IWebService(broker)
    {
    }

  public:
    static WasmWebService& GetInstance()
    {
      if (broker_ == NULL)
      {
        printf("WasmWebService::GetInstance(): broker not initialized\n");
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      static WasmWebService instance(*broker_);
      return instance;
    }

    static void SetBroker(MessageBroker& broker)
    {
      broker_ = &broker;
    }

    virtual void GetAsync(const std::string& uri,
                          const HttpHeaders& headers,
                          Orthanc::IDynamicObject* payload,
                          MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallable,
                          MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallable = NULL,
                          unsigned int timeoutInSeconds = 60);

    virtual void PostAsync(const std::string& uri,
                           const HttpHeaders& headers,
                           const std::string& body,
                           Orthanc::IDynamicObject* payload,
                           MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallable,
                           MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallable = NULL,
                           unsigned int timeoutInSeconds = 60);

    virtual void DeleteAsync(const std::string& uri,
                             const HttpHeaders& headers,
                             Orthanc::IDynamicObject* payload,
                             MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallable,
                             MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallable = NULL,
                             unsigned int timeoutInSeconds = 60);

    // virtual void Start()
    // {
    // }
    
    // virtual void Stop()
    // {
    // }
  };
}
