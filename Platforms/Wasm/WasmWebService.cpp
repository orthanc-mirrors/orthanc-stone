#include "WasmWebService.h"
#include "json/value.h"
#include "json/writer.h"
#include <emscripten/emscripten.h>
#include <boost/shared_ptr.hpp>

struct CachedSuccessNotification
{
  boost::shared_ptr<Deprecated::BaseWebService::CachedHttpRequestSuccessMessage>    cachedMessage;
  std::auto_ptr<Orthanc::IDynamicObject>                                              payload;
  OrthancStone::MessageHandler<Deprecated::IWebService::HttpRequestSuccessMessage>* successCallback;
};


#ifdef __cplusplus
extern "C" {
#endif

  extern void WasmWebService_GetAsync(void* callableSuccess,
                                      void* callableFailure,
                                      const char* uri,
                                      const char* headersInJsonString,
                                      void* payload,
                                      unsigned int timeoutInSeconds);

  extern void WasmWebService_ScheduleLaterCachedSuccessNotification(void* brol);

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
                                                       unsigned int httpStatus,
                                                       void* payload)
  {
    if (failureCallable != NULL)
    {
      reinterpret_cast<OrthancStone::MessageHandler<Deprecated::IWebService::HttpRequestErrorMessage>*>(failureCallable)->
        Apply(Deprecated::IWebService::HttpRequestErrorMessage(uri, static_cast<Orthanc::HttpStatus>(httpStatus), reinterpret_cast<Orthanc::IDynamicObject*>(payload)));
    }
  }

  void EMSCRIPTEN_KEEPALIVE WasmWebService_NotifyCachedSuccess(void* notification_)
  {
    // notification has been allocated in C++ and passed to JS.  It must be deleted by this method
    std::auto_ptr<CachedSuccessNotification> notification(reinterpret_cast<CachedSuccessNotification*>(notification_));

    notification->successCallback->Apply(Deprecated::IWebService::HttpRequestSuccessMessage(
      notification->cachedMessage->GetUri(), 
      notification->cachedMessage->GetAnswer(),
      notification->cachedMessage->GetAnswerSize(),
      notification->cachedMessage->GetAnswerHttpHeaders(),
      notification->payload.get()
      ));
  }

  void EMSCRIPTEN_KEEPALIVE WasmWebService_NotifySuccess(void* successCallable,
                                                         const char* uri,
                                                         const void* body,
                                                         size_t bodySize,
                                                         const char* answerHeaders,
                                                         void* payload)
  {
    if (successCallable != NULL)
    {
      Deprecated::IWebService::HttpHeaders headers;

      // TODO - Parse "answerHeaders"
      //printf("TODO: parse headers [%s]\n", answerHeaders);
      
      reinterpret_cast<OrthancStone::MessageHandler<Deprecated::IWebService::HttpRequestSuccessMessage>*>(successCallable)->
        Apply(Deprecated::IWebService::HttpRequestSuccessMessage(uri, body, bodySize, headers,
                                                                   reinterpret_cast<Orthanc::IDynamicObject*>(payload)));
    }
  }

#ifdef __cplusplus
}
#endif



namespace Deprecated
{
  OrthancStone::MessageBroker* WasmWebService::broker_ = NULL;

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
                                 OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallable,
                                 OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallable,
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
                                   OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallable,
                                   OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage>* failureCallable,
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
                                        OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage> *successCallable,
                                        OrthancStone::MessageHandler<IWebService::HttpRequestErrorMessage> *failureCallable,
                                        unsigned int timeoutInSeconds)
  {
    std::string headersInJsonString;
    ToJsonString(headersInJsonString, headers);
    WasmWebService_GetAsync(successCallable, failureCallable, relativeUri.c_str(),
                            headersInJsonString.c_str(), payload, timeoutInSeconds);
  }

  void WasmWebService::NotifyHttpSuccessLater(boost::shared_ptr<BaseWebService::CachedHttpRequestSuccessMessage> cachedMessage,
                                              Orthanc::IDynamicObject* payload, // takes ownership
                                              OrthancStone::MessageHandler<IWebService::HttpRequestSuccessMessage>* successCallback)
  {
    CachedSuccessNotification* notification = new CachedSuccessNotification();  // allocated on the heap, it will be passed to JS and deleted when coming back to C++
    notification->cachedMessage = cachedMessage;
    notification->payload.reset(payload);
    notification->successCallback = successCallback;

    WasmWebService_ScheduleLaterCachedSuccessNotification(notification);
  }

}
