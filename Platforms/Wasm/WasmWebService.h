#pragma once

#include <Framework/Toolbox/IWebService.h>

namespace OrthancStone
{
  class WasmWebService : public IWebService
  {
  private:
    std::string  base_;

    // Private constructor => Singleton design pattern
    WasmWebService() :
      base_("../../")
    {
    }

  public:
    static WasmWebService& GetInstance()
    {
      static WasmWebService instance;
      return instance;
    }

    void SetBaseUrl(const std::string base);

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
