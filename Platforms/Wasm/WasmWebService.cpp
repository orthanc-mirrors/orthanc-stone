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
        OnHttpRequestError(uri, reinterpret_cast<Orthanc::IDynamicObject*>(payload));
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
        OnHttpRequestSuccess(uri, body, bodySize, reinterpret_cast<Orthanc::IDynamicObject*>(payload)); 
   }
  }

  void EMSCRIPTEN_KEEPALIVE WasmWebService_SetBaseUri(const char* baseUri)
  {
    OrthancStone::WasmWebService::GetInstance().SetBaseUri(baseUri);
  }

#ifdef __cplusplus
}
#endif



namespace OrthancStone
{
  MessageBroker* WasmWebService::broker_ = NULL;

  void WasmWebService::SetBaseUri(const std::string baseUri)
  {
    // Make sure the base url ends with "/"
    if (baseUri.empty() ||
        baseUri[baseUri.size() - 1] != '/')
    {
      baseUri_ = baseUri + "/";
    }
    else
    {
      baseUri_ = baseUri;
    }
  }

  void WasmWebService::ScheduleGetRequest(ICallback& callback,
                                          const std::string& relativeUri,
                                          Orthanc::IDynamicObject* payload)
  {
    std::string uri = baseUri_ + relativeUri;
    WasmWebService_ScheduleGetRequest(&callback, uri.c_str(), payload);
  }

  void WasmWebService::SchedulePostRequest(ICallback& callback,
                                           const std::string& relativeUri,
                                           const std::string& body,
                                           Orthanc::IDynamicObject* payload)
  {
    std::string uri = baseUri_ + relativeUri;
    WasmWebService_SchedulePostRequest(&callback, uri.c_str(),
                                       body.c_str(), body.size(), payload);
  }
}
