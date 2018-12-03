#include "WasmWebService.h"
#include "json/value.h"
#include "json/writer.h"
#include <emscripten/emscripten.h>

#ifdef __cplusplus
extern "C" {
#endif

  extern void WasmWebService_GetAsync(void* callableSuccess,
                                      void* callableFailure,
                                      const char* uri,
                                      const char* headersInJsonString,
                                      void* payload,
                                      unsigned int timeoutInSeconds);

  extern void WasmWebService_PostAsync(void* callableSuccess,
                                       void* callableFailure,
                                       const char* uri,
                                       const char* headersInJsonString,
                                       const void* body,
                                       size_t bodySize,
                                       void* payload,
                                       unsigned int timeoutInSeconds);

  extern void WasmWebService_DeleteAsync(void* callableSuccess,
                                         void* callableFailure,
                                         const char* uri,
                                         const char* headersInJsonString,
                                         void* payload,
                                         unsigned int timeoutInSeconds);

  void EMSCRIPTEN_KEEPALIVE WasmWebService_NotifyError(void* failureCallable,
                                                       const char* uri,
                                                       void* payload)
  {
    if (failureCallable == NULL)
    {
      throw;
    }
    else
    {
      reinterpret_cast<OrthancStone::MessageHandler<OrthancStone::IWebService::HttpRequestErrorMessage>*>(failureCallable)->
        Apply(OrthancStone::IWebService::HttpRequestErrorMessage(uri, reinterpret_cast<Orthanc::IDynamicObject*>(payload)));
    }
  }

  void EMSCRIPTEN_KEEPALIVE WasmWebService_NotifySuccess(void* successCallable,
                                                         const char* uri,
                                                         const void* body,
                                                         size_t bodySize,
                                                         const char* answerHeaders,
                                                         void* payload)
  {
    if (successCallable == NULL)
    {
      throw;
    }
    else
    {
      OrthancStone::IWebService::HttpHeaders headers;

      // TODO - Parse "answerHeaders"
      printf("[%s]\n", answerHeaders);
      
      reinterpret_cast<OrthancStone::MessageHandler<OrthancStone::IWebService::HttpRequestSuccessMessage>*>(successCallable)->
        Apply(OrthancStone::IWebService::HttpRequestSuccessMessage(uri, body, bodySize, headers,
                                                                   reinterpret_cast<Orthanc::IDynamicObject*>(payload)));
    }
  }

#ifdef __cplusplus
}
#endif



namespace OrthancStone
{
  MessageBroker* WasmWebService::broker_ = NULL;

  void ToJsonString(std::string& output, const IWebService::HttpHeaders& headers)
  {
    Json::Value jsonHeaders;
    for (IWebService::HttpHeaders::const_iterator it = headers.begin(); it != headers.end(); it++ )
    {
      jsonHeaders[it->first] = it->second;
    }

    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream outputStr;

    writer->write(jsonHeaders, &outputStr);
    output = outputStr.str();
  }

  void WasmWebService::PostAsync(const std::string& relativeUri,
                                 const HttpHeaders& headers,
                                 const std::string& body,
                                 Orthanc::IDynamicObject* payload,
                                 MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallable,
                                 MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallable,
                                 unsigned int timeoutInSeconds)
  {
    std::string headersInJsonString;
    ToJsonString(headersInJsonString, headers);
    WasmWebService_PostAsync(successCallable, failureCallable, relativeUri.c_str(), headersInJsonString.c_str(),
                             body.c_str(), body.size(), payload, timeoutInSeconds);
  }

  void WasmWebService::DeleteAsync(const std::string& relativeUri,
                                   const HttpHeaders& headers,
                                   Orthanc::IDynamicObject* payload,
                                   MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallable,
                                   MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallable,
                                   unsigned int timeoutInSeconds)
  {
    std::string headersInJsonString;
    ToJsonString(headersInJsonString, headers);
    WasmWebService_DeleteAsync(successCallable, failureCallable, relativeUri.c_str(), headersInJsonString.c_str(),
                               payload, timeoutInSeconds);
  }

  void WasmWebService::GetAsyncInternal(const std::string &relativeUri,
                                        const HttpHeaders &headers,
                                        Orthanc::IDynamicObject *payload,
                                        MessageHandler<IWebService::HttpRequestSuccessMessage> *successCallable,
                                        MessageHandler<IWebService::HttpRequestErrorMessage> *failureCallable,
                                        unsigned int timeoutInSeconds)
  {
    std::string headersInJsonString;
    ToJsonString(headersInJsonString, headers);
    WasmWebService_GetAsync(successCallable, failureCallable, relativeUri.c_str(),
                            headersInJsonString.c_str(), payload, timeoutInSeconds);
  }
}
