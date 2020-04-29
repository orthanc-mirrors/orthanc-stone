#pragma once

#include "../../Framework/Deprecated/Toolbox/BaseWebService.h"
#include <Core/OrthancException.h>

namespace Deprecated
{
class WasmWebService : public BaseWebService
{
private:
  static OrthancStone::MessageBroker *broker_;

  // Private constructor => Singleton design pattern
  WasmWebService(OrthancStone::MessageBroker &broker) : BaseWebService(broker)
  {
  }

public:
  static WasmWebService &GetInstance()
  {
    if (broker_ == NULL)
    {
      printf("WasmWebService::GetInstance(): broker not initialized\n");
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    static WasmWebService instance(*broker_);
    return instance;
  }

  static void SetBroker(OrthancStone::MessageBroker &broker)
  {
    broker_ = &broker;
  }

  virtual void PostAsync(const std::string &uri,
                         const HttpHeaders &headers,
                         const std::string &body,
                         Orthanc::IDynamicObject *payload,
                         OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage> *successCallable,
                         OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage> *failureCallable = NULL,
                         unsigned int timeoutInSeconds = 60);

  virtual void DeleteAsync(const std::string &uri,
                           const HttpHeaders &headers,
                           Orthanc::IDynamicObject *payload,
                           OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage> *successCallable,
                           OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage> *failureCallable = NULL,
                           unsigned int timeoutInSeconds = 60);

protected:
  virtual void GetAsyncInternal(const std::string &uri,
                                const HttpHeaders &headers,
                                Orthanc::IDynamicObject *payload,
                                OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage> *successCallable,
                                OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage> *failureCallable = NULL,
                                unsigned int timeoutInSeconds = 60);

  virtual void NotifyHttpSuccessLater(boost::shared_ptr<BaseWebService::CachedHttpRequestSuccessMessage> cachedHttpMessage,
                                      Orthanc::IDynamicObject *payload, // takes ownership
                                      OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage> *successCallback);
};
} // namespace Deprecated
