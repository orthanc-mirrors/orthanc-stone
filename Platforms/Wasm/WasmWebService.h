#pragma once

#include <Framework/Toolbox/IWebService.h>
#include <Core/OrthancException.h>

namespace OrthancStone
{
  class WasmWebService : public IWebService
  {
  private:
    std::string  baseUri_;
    static MessageBroker* broker_;

    // Private constructor => Singleton design pattern
    WasmWebService(MessageBroker& broker) :
      IWebService(broker),
      baseUri_("../../")   // note: this is configurable from the JS code by calling WasmWebService_SetBaseUri
    {
    }

  public:
    static WasmWebService& GetInstance()
    {
      if (broker_ == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      static WasmWebService instance(*broker_);
      return instance;
    }

    static void SetBroker(MessageBroker& broker)
    {
      broker_ = &broker;
    }

    void SetBaseUri(const std::string baseUri);

    virtual void ScheduleGetRequest(ICallback& callback,
                                    const std::string& uri,
                                    Orthanc::IDynamicObject* payload);

    virtual void SchedulePostRequest(ICallback& callback,
                                     const std::string& uri,
                                     const std::string& body,
                                     Orthanc::IDynamicObject* payload);

    virtual void Start()
    {
    }
    
    virtual void Stop()
    {
    }
  };
}
