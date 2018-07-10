#include "WasmWebService.h"
#include "json/value.h"
#include "json/writer.h"
#include <emscripten/emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

  extern void WasmWebService_ScheduleGetRequest(void* callback,
                                                const char* uri,
                                                const char* headersInJsonString,
                                                void* payload);
  
  extern void WasmWebService_SchedulePostRequest(void* callback,
                                                 const char* uri,
                                                 const char* headersInJsonString,
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

  void ToJsonString(std::string& output, const IWebService::Headers& headers)
  {
    Json::Value jsonHeaders;
    for (IWebService::Headers::const_iterator it = headers.begin(); it != headers.end(); it++ )
    {
      jsonHeaders[it->first] = it->second;
    }

    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream outputStr;

    writer->write(jsonHeaders, &outputStr);
    output = outputStr.str();
  }

  void WasmWebService::ScheduleGetRequest(ICallback& callback,
                                          const std::string& relativeUri,
                                          const Headers& headers,
                                          Orthanc::IDynamicObject* payload)
  {
    std::string uri = baseUri_ + relativeUri;
    std::string headersInJsonString;
    ToJsonString(headersInJsonString, headers);
    WasmWebService_ScheduleGetRequest(&callback, uri.c_str(), headersInJsonString.c_str(), payload);
  }

  void WasmWebService::SchedulePostRequest(ICallback& callback,
                                           const std::string& relativeUri,
                                           const Headers& headers,
                                           const std::string& body,
                                           Orthanc::IDynamicObject* payload)
  {
    std::string uri = baseUri_ + relativeUri;
    std::string headersInJsonString;
    ToJsonString(headersInJsonString, headers);
    WasmWebService_SchedulePostRequest(&callback, uri.c_str(), headersInJsonString.c_str(),
                                       body.c_str(), body.size(), payload);
  }
}
