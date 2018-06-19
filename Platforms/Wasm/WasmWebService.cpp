#include "WasmWebService.h"

#include <emscripten/emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

  extern void WasmWebService_ScheduleGetRequest(void* callback,
                                                const char* uri,
                                                void* payload);
  
  extern void WasmWebService_SchedulePostRequest(void* callback,
                                                 const char* uri,
                                                 const void* body,
                                                 size_t bodySize,
                                                 void* payload);

  void EMSCRIPTEN_KEEPALIVE WasmWebService_NotifyError(void* callback,
                                                       const char* uri,
                                                       void* payload)
  {
    if (callback == NULL)
    {
      throw;
    }
    else
    {
      reinterpret_cast<OrthancStone::IWebService::ICallback*>(callback)->
        NotifyError(uri, reinterpret_cast<Orthanc::IDynamicObject*>(payload));
    }
  }

  void EMSCRIPTEN_KEEPALIVE WasmWebService_NotifySuccess(void* callback,
                                                         const char* uri,
                                                         const void* body,
                                                         size_t bodySize,
                                                         void* payload)
  {
    if (callback == NULL)
    {
      throw;
    }
    else
    {
      reinterpret_cast<OrthancStone::IWebService::ICallback*>(callback)->
        NotifySuccess(uri, body, bodySize, reinterpret_cast<Orthanc::IDynamicObject*>(payload)); 
   }
  }

#ifdef __cplusplus
}
#endif



namespace OrthancStone
{
  void WasmWebService::SetBaseUrl(const std::string base)
  {
    // Make sure the base url ends with "/"
    if (base.empty() ||
        base[base.size() - 1] != '/')
    {
      base_ = base + "/";
    }
    else
    {
      base_ = base;
    }
  }

  void WasmWebService::ScheduleGetRequest(ICallback& callback,
                                          const std::string& uri,
                                          Orthanc::IDynamicObject* payload)
  {
    std::string url = base_ + uri;
    WasmWebService_ScheduleGetRequest(&callback, url.c_str(), payload);
  }

  void WasmWebService::SchedulePostRequest(ICallback& callback,
                                           const std::string& uri,
                                           const std::string& body,
                                           Orthanc::IDynamicObject* payload)
  {
    std::string url = base_ + uri;
    WasmWebService_SchedulePostRequest(&callback, url.c_str(),
                                       body.c_str(), body.size(), payload);
  }
}
