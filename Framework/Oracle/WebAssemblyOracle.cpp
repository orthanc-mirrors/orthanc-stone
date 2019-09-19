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


#include "WebAssemblyOracle.h"

#include "SleepOracleCommand.h"
#include "OracleCommandExceptionMessage.h"

#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>

#if 0
extern bool logbgo233;
extern bool logbgo115;
#endif

namespace OrthancStone
{
  class WebAssemblyOracle::TimeoutContext
  {
  private:
    WebAssemblyOracle&                 oracle_;
    const IObserver&                   receiver_;
    std::auto_ptr<SleepOracleCommand>  command_;

  public:
    TimeoutContext(WebAssemblyOracle& oracle,
                   const IObserver& receiver,
                   IOracleCommand* command) :
      oracle_(oracle),
      receiver_(receiver)
    {
      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      else
      {
        command_.reset(dynamic_cast<SleepOracleCommand*>(command));
      }
    }

    void EmitMessage()
    {
      SleepOracleCommand::TimeoutMessage message(*command_);
      oracle_.EmitMessage(receiver_, message);
    }

    static void Callback(void *userData)
    {
      std::auto_ptr<TimeoutContext> context(reinterpret_cast<TimeoutContext*>(userData));
      context->EmitMessage();
    }
  };
    

  class WebAssemblyOracle::Emitter : public IMessageEmitter
  {
  private:
    WebAssemblyOracle&  oracle_;

  public:
    Emitter(WebAssemblyOracle&  oracle) :
      oracle_(oracle)
    {
    }

    virtual void EmitMessage(const IObserver& receiver,
                             const IMessage& message)
    {
      LOG(TRACE) << "WebAssemblyOracle::Emitter::EmitMessage receiver = "
        << std::hex << &receiver << std::dec;
      oracle_.EmitMessage(receiver, message);
    }
  };

  /**
  This object is created on the heap for every http request.
  It is deleted in the success (or error) callbacks.

  This object references the receiver of the request. Since this is a raw
  reference, we need additional checks to make sure we send the response to
  the same object, for the object can be deleted and a new one recreated at the
  same address (it often happens in the [single-threaded] browser context).
  */
  class WebAssemblyOracle::FetchContext : public boost::noncopyable
  {
  private:
    Emitter                        emitter_;
    const IObserver&               receiver_;
    std::auto_ptr<IOracleCommand>  command_;
    std::string                    expectedContentType_;
    std::string                    receiverFingerprint_;

  public:
    FetchContext(WebAssemblyOracle& oracle,
                 const IObserver& receiver,
                 IOracleCommand* command,
                 const std::string& expectedContentType) :
      emitter_(oracle),
      receiver_(receiver),
      command_(command),
      expectedContentType_(expectedContentType),
      receiverFingerprint_(receiver.GetFingerprint())
    {
      LOG(TRACE) << "WebAssemblyOracle::FetchContext::FetchContext() | "
        << "receiver address = " << std::hex << &receiver << std::dec 
        << " with fingerprint = " << receiverFingerprint_;

      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    const std::string& GetExpectedContentType() const
    {
      return expectedContentType_;
    }

    void EmitMessage(const IMessage& message)
    {
      LOG(TRACE) << "WebAssemblyOracle::FetchContext::EmitMessage receiver_ = "
        << std::hex << &receiver_ << std::dec;
      emitter_.EmitMessage(receiver_, message);
    }

    IMessageEmitter& GetEmitter()
    {
      return emitter_;
    }

    const IObserver& GetReceiver() const
    {
      return receiver_;
    }

    IOracleCommand& GetCommand() const
    {
      return *command_;
    }

    template <typename T>
    const T& GetTypedCommand() const
    {
      return dynamic_cast<T&>(*command_);
    }

#if 0
    static std::string ToString(Orthanc::HttpMethod method)
    {
      switch (method) {
      case Orthanc::HttpMethod_Get:
        return "GET";
        break;
      case Orthanc::HttpMethod_Post:
        return "POST";
        break;
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        break;
      }
    }
    static void DumpCommand(emscripten_fetch_t* fetch, std::string answer)
    {
      FetchContext* context = reinterpret_cast<FetchContext*>(fetch->userData);

      const auto& command = context->GetTypedCommand<OrthancRestApiCommand>();
      auto commandStr = ToString(command.GetMethod());
      LOG(TRACE) << "SuccessCallback for REST command. Method is : " << commandStr;
      switch (command.GetMethod()) {
      case Orthanc::HttpMethod_Get:
        LOG(TRACE) << "  * SuccessCallback GET URI = " << command.GetUri() << " timeout = " << command.GetTimeout();
        LOG(TRACE) << "  * SuccessCallback GET RESPONSE = " << answer;
        break;
      case Orthanc::HttpMethod_Post:
        LOG(TRACE) << "  * SuccessCallback POST URI = " << command.GetUri() << " body = " << command.GetBody() << " timeout = " << command.GetTimeout();
        LOG(TRACE) << "  * SuccessCallback POST RESPONSE = " << answer;
        break;
      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        break;
      }
    }
#endif

    static void SuccessCallback(emscripten_fetch_t *fetch)
    {
      /**
       * Firstly, make a local copy of the fetched information, and
       * free data associated with the fetch.
       **/
      
      std::auto_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));

      // an UUID is 36 chars : 32 hex chars + 4 hyphens: char #0 --> char #35
      // char #36 is \0.
      bool callHandler = true;
      
      // TODO: remove this line because we are NOT allowed to call methods on GetReceiver that is maybe a dangling ref
      if (context->GetReceiver().DoesFingerprintLookGood())
      {
        callHandler = true;
        std::string currentFingerprint(context->GetReceiver().GetFingerprint());
       
        LOG(TRACE) << "SuccessCallback for object at address (" << std::hex 
          << &(context->GetReceiver()) << std::dec 
          << " with current fingerprint = " << currentFingerprint 
          << ". Fingerprint looks OK";
        
        if (currentFingerprint != context->receiverFingerprint_)
        {
          LOG(TRACE) << "  ** SuccessCallback: BUT currentFingerprint != "
            << "receiverFingerprint_(" << context->receiverFingerprint_ << ")";
          callHandler = false;
        }
        else
        {
          LOG(TRACE) << "  ** SuccessCallback: FetchContext-level "
            << "fingerprints are the same: "
            << context->receiverFingerprint_
            << " ---> oracle will dispatch the response to observer: "
            << std::hex << &(context->GetReceiver()) << std::dec;
        }
      }
      else {
        LOG(TRACE) << "SuccessCallback for object at address (" << std::hex << &(context->GetReceiver()) << std::dec << " with current fingerprint is XXXXX -- NOT A VALID FINGERPRINT! OBJECT IS READ ! CALLBACK WILL NOT BE CALLED!";
        callHandler = false;
      }

      if (fetch->userData == NULL)
      {
        LOG(ERROR) << "WebAssemblyOracle::FetchContext::SuccessCallback fetch->userData is NULL!!!!!!!";
      }

      std::string answer;
      if (fetch->numBytes > 0)
      {
        answer.assign(fetch->data, fetch->numBytes);
      }

      /**
       * TODO - HACK - As of emscripten-1.38.31, the fetch API does
       * not contain a way to retrieve the HTTP headers of the
       * answer. We make the assumption that the "Content-Type" header
       * of the response is the same as the "Accept" header of the
       * query. This should be fixed in future versions of emscripten.
       * https://github.com/emscripten-core/emscripten/pull/8486
       **/

      HttpHeaders headers;
      if (fetch->userData != NULL)
      {
        if (!context->GetExpectedContentType().empty())
        {
          headers["Content-Type"] = context->GetExpectedContentType();
        }
      }
      
#if 0
      if (context->GetCommand().GetType() == IOracleCommand::Type_OrthancRestApi) {
        //if (logbgo115)
        DumpCommand(fetch, answer);
      }
#endif
      LOG(TRACE) << "About to call emscripten_fetch_close";
      emscripten_fetch_close(fetch);
      LOG(TRACE) << "Successfully called emscripten_fetch_close";

      /**
       * Secondly, use the retrieved data.
       * IMPORTANT NOTE: the receiver might be dead. This is prevented 
       * by the object responsible for zombie check, later on.
       **/
      try
      {
        if (context.get() == NULL)
        {
          LOG(ERROR) << "WebAssemblyOracle::FetchContext::SuccessCallback: (context.get() == NULL)";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
        else
        {
          if (callHandler)
          {
            switch (context->GetCommand().GetType())
            {
              case IOracleCommand::Type_Http:
              {
                HttpCommand::SuccessMessage message(context->GetTypedCommand<HttpCommand>(), headers, answer);
                context->EmitMessage(message);
                break;
              }

              case IOracleCommand::Type_OrthancRestApi:
              {
                LOG(TRACE) << "WebAssemblyOracle::FetchContext::SuccessCallback. About to call context->EmitMessage(message);";
                OrthancRestApiCommand::SuccessMessage message
                  (context->GetTypedCommand<OrthancRestApiCommand>(), headers, answer);
                context->EmitMessage(message);
                break;
              }

              case IOracleCommand::Type_GetOrthancImage:
              {
                context->GetTypedCommand<GetOrthancImageCommand>().ProcessHttpAnswer
                  (context->GetEmitter(), context->GetReceiver(), answer, headers);
                break;
              }

              case IOracleCommand::Type_GetOrthancWebViewerJpeg:
              {
                context->GetTypedCommand<GetOrthancWebViewerJpegCommand>().ProcessHttpAnswer
                  (context->GetEmitter(), context->GetReceiver(), answer);
                break;
              }

              default:
                LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle: "
                           << context->GetCommand().GetType();
            }
          }
        }
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Error while processing a fetch answer in the oracle: " << e.What();
      }
    }

    static void FailureCallback(emscripten_fetch_t *fetch)
    {
      std::auto_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));

      {
        const size_t kEmscriptenStatusTextSize = sizeof(emscripten_fetch_t::statusText);
        char message[kEmscriptenStatusTextSize + 1];
        memcpy(message, fetch->statusText, kEmscriptenStatusTextSize);
        message[kEmscriptenStatusTextSize] = 0;

        /*LOG(ERROR) << "Fetching " << fetch->url
                   << " failed, HTTP failure status code: " << fetch->status
                   << " | statusText = " << message
                   << " | numBytes = " << fetch->numBytes
                   << " | totalBytes = " << fetch->totalBytes
                   << " | readyState = " << fetch->readyState;*/
      }

      {
        OracleCommandExceptionMessage message
          (context->GetCommand(), Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol));
        context->EmitMessage(message);
      }
      
      /**
       * TODO - The following code leads to an infinite recursion, at
       * least with Firefox running on incognito mode => WHY?
       **/      
      emscripten_fetch_close(fetch); // Also free data on failure.
    }
  };
    


  class WebAssemblyOracle::FetchCommand : public boost::noncopyable
  {
  private:
    WebAssemblyOracle&             oracle_;
    const IObserver&               receiver_;
    std::auto_ptr<IOracleCommand>  command_;
    Orthanc::HttpMethod            method_;
    std::string                    url_;
    std::string                    body_;
    HttpHeaders                    headers_;
    unsigned int                   timeout_;
    std::string                    expectedContentType_;

  public:
    FetchCommand(WebAssemblyOracle& oracle,
                 const IObserver& receiver,
                 IOracleCommand* command) :
      oracle_(oracle),
      receiver_(receiver),
      command_(command),
      method_(Orthanc::HttpMethod_Get),
      timeout_(0)
    {
      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    void SetMethod(Orthanc::HttpMethod method)
    {
      method_ = method;
    }

    void SetOrthancUri(const std::string& uri)
    {
      url_ = oracle_.orthancRoot_ + uri;
    }

    void SetUrl(const std::string& url)
    {
      url_ = url;
    }

    void SetBody(std::string& body /* will be swapped */)
    {
      body_.swap(body);
    }

    void SetHttpHeaders(const HttpHeaders& headers)
    {
      headers_ = headers;
    }

    void SetTimeout(unsigned int timeout)
    {
      timeout_ = timeout;
    }

    void Execute()
    {
#if 0
      if (logbgo233) {
        if (logbgo115)
          LOG(TRACE) << "        WebAssemblyOracle::Execute () command addr " <<
          std::hex << command_.get() << std::dec;
      }
#endif
      if (command_.get() == NULL)
      {
        // Cannot call Execute() twice
        LOG(ERROR) << "WebAssemblyOracle::Execute(): (command_.get() == NULL)";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      emscripten_fetch_attr_t attr;
      emscripten_fetch_attr_init(&attr);

      const char* method;
      
      switch (method_)
      {
        case Orthanc::HttpMethod_Get:
          method = "GET";
          break;

        case Orthanc::HttpMethod_Post:
          method = "POST";
          break;

        case Orthanc::HttpMethod_Delete:
          method = "DELETE";
          break;

        case Orthanc::HttpMethod_Put:
          method = "PUT";
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      strcpy(attr.requestMethod, method);

      attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_REPLACE;
      attr.onsuccess = FetchContext::SuccessCallback;
      attr.onerror = FetchContext::FailureCallback;
      attr.timeoutMSecs = timeout_ * 1000;

      std::vector<const char*> headers;
      headers.reserve(2 * headers_.size() + 1);

      std::string expectedContentType;
        
      for (HttpHeaders::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
      {
        std::string key;
        Orthanc::Toolbox::ToLowerCase(key, it->first);
          
        if (key == "accept")
        {
          expectedContentType = it->second;
        }

        if (key != "accept-encoding")  // Web browsers forbid the modification of this HTTP header
        {
          headers.push_back(it->first.c_str());
          headers.push_back(it->second.c_str());
        }
      }
        
      headers.push_back(NULL);  // Termination of the array of HTTP headers

      attr.requestHeaders = &headers[0];

      char* requestData = NULL;
      if (!body_.empty())
        requestData = reinterpret_cast<char*>(malloc(body_.size()));
        
      try 
      {
        if (!body_.empty())
        {
          memcpy(requestData, &(body_[0]), body_.size());
          attr.requestDataSize = body_.size();
          attr.requestData = requestData;
        }
        attr.userData = new FetchContext(oracle_, receiver_, command_.release(), expectedContentType);

        // Must be the last call to prevent memory leak on error
#if 0
        LOG(TRACE) << "Performing " << method << " request on URI: \"" << url_ << "\"";
#endif
        emscripten_fetch(&attr, url_.c_str());
      }        
      catch(...)
      {
        if(requestData != NULL)
          free(requestData);
        throw;
      }
    }
  };

#if 0
  static void DumpCommand(OrthancRestApiCommand* pCommand)
  {
    OrthancRestApiCommand& command = *pCommand;
    LOG(TRACE) << "WebAssemblyOracle::Execute for REST command.";
    switch (command.GetMethod()) {
    case Orthanc::HttpMethod_Get:
      LOG(TRACE) << "  * WebAssemblyOracle::Execute GET URI = " << command.GetUri() << " timeout = " << command.GetTimeout();
      break;
    case Orthanc::HttpMethod_Post:
      LOG(TRACE) << "  * WebAssemblyOracle::Execute POST URI = " << command.GetUri() << " body = " << command.GetBody() << " timeout = " << command.GetTimeout();
      break;
    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      break;
    }
  }
#endif

  
  void WebAssemblyOracle::Execute(const IObserver& receiver,
                                  HttpCommand* command)
  {
    FetchCommand fetch(*this, receiver, command);
    
    fetch.SetMethod(command->GetMethod());
    fetch.SetUrl(command->GetUrl());
    fetch.SetHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
    
    if (command->GetMethod() == Orthanc::HttpMethod_Post ||
        command->GetMethod() == Orthanc::HttpMethod_Put)
    {
      std::string body;
      command->SwapBody(body);
      fetch.SetBody(body);
    }
    
    fetch.Execute();
  }
  

  void WebAssemblyOracle::Execute(const IObserver& receiver,
                                  OrthancRestApiCommand* command)
  {
#if 0
    DumpCommand(command);

    if (logbgo233) {
      if (logbgo115)
        LOG(TRACE) << "        WebAssemblyOracle::Execute (OrthancRestApiCommand) command addr " <<
        std::hex << command << std::dec;
    }
#endif

    try
    {
      //LOG(TRACE) << "*********** WebAssemblyOracle::Execute.";
      //LOG(TRACE) << "WebAssemblyOracle::Execute | command = " << command;
      FetchCommand fetch(*this, receiver, command);

      fetch.SetMethod(command->GetMethod());
      fetch.SetOrthancUri(command->GetUri());
      fetch.SetHttpHeaders(command->GetHttpHeaders());
      fetch.SetTimeout(command->GetTimeout());

      if (command->GetMethod() == Orthanc::HttpMethod_Post ||
        command->GetMethod() == Orthanc::HttpMethod_Put)
      {
        std::string body;
        command->SwapBody(body);
        fetch.SetBody(body);
      }

      fetch.Execute();
      //LOG(TRACE) << "*********** successful end of WebAssemblyOracle::Execute.";
    }
    catch (const Orthanc::OrthancException& e)
    {
      if (e.HasDetails())
      {
        LOG(ERROR) << "OrthancException in WebAssemblyOracle::Execute: " << e.What() << " Details: " << e.GetDetails();
      }
      else
      {
        LOG(ERROR) << "OrthancException in WebAssemblyOracle::Execute: " << e.What();
      }
      //LOG(TRACE) << "*********** failing end of WebAssemblyOracle::Execute.";
      throw;
    }
    catch (const std::exception& e)
    {
      LOG(ERROR) << "std::exception in WebAssemblyOracle::Execute: " << e.what();
//       LOG(TRACE) << "*********** failing end of WebAssemblyOracle::Execute.";
      throw;
    }
    catch (...)
    {
      LOG(ERROR) << "Unknown exception in WebAssemblyOracle::Execute";
//       LOG(TRACE) << "*********** failing end of WebAssemblyOracle::Execute.";
      throw;
    }
  }
    
    
  void WebAssemblyOracle::Execute(const IObserver& receiver,
                                  GetOrthancImageCommand* command)
  {
#if 0
    if (logbgo233) {
      if (logbgo115)
        LOG(TRACE) << "        WebAssemblyOracle::Execute (GetOrthancImageCommand) command addr " <<
        std::hex << command << std::dec;
    }
#endif

    FetchCommand fetch(*this, receiver, command);

    fetch.SetOrthancUri(command->GetUri());
    fetch.SetHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
      
    fetch.Execute();
  }
    
    
  void WebAssemblyOracle::Execute(const IObserver& receiver,
                                  GetOrthancWebViewerJpegCommand* command)
  {
#if 0
    if (logbgo233) {
      if (logbgo115)
        LOG(TRACE) << "        WebAssemblyOracle::Execute (GetOrthancWebViewerJpegCommand) command addr " << std::hex << command << std::dec;
    }
#endif

    FetchCommand fetch(*this, receiver, command);

    fetch.SetOrthancUri(command->GetUri());
    fetch.SetHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
      
    fetch.Execute();
  }



  void WebAssemblyOracle::Schedule(const IObserver& receiver,
                                   IOracleCommand* command)
  {
    LOG(TRACE) << "WebAssemblyOracle::Schedule : receiver = "
      << std::hex << &receiver << std::dec
      << " | Current fingerprint is " << receiver.GetFingerprint();
    
    std::auto_ptr<IOracleCommand> protection(command);

    if (command == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    switch (command->GetType())
    {
      case IOracleCommand::Type_Http:
        Execute(receiver, dynamic_cast<HttpCommand*>(protection.release()));
        break;
        
      case IOracleCommand::Type_OrthancRestApi:
        //// DIAGNOSTIC. PLEASE REMOVE IF IT HAS BEEN COMMITTED BY MISTAKE
        //{
        //  const IObserver* pReceiver = &receiver;
        //  LOG(TRACE) << "WebAssemblyOracle::Schedule | pReceiver is " << pReceiver;
        //  LOG(TRACE) << "WebAssemblyOracle::Schedule | command = " << command;
        //  OrthancRestApiCommand* rac = dynamic_cast<OrthancRestApiCommand*>(protection.get());
        //  LOG(TRACE) << "WebAssemblyOracle::Schedule | typed command = " << rac;
        //  LOG(TRACE) << "WebAssemblyOracle::Schedule" << rac->GetUri();
        //}
        //// END OF BLOCK TO REMOVE
        Execute(receiver, dynamic_cast<OrthancRestApiCommand*>(protection.release()));
        break;
        
      case IOracleCommand::Type_GetOrthancImage:
        //// DIAGNOSTIC. PLEASE REMOVE IF IT HAS BEEN COMMITTED BY MISTAKE
        //{
        //  GetOrthancImageCommand* rac = dynamic_cast<GetOrthancImageCommand*>(protection.get());
        //  LOG(TRACE) << "WebAssemblyOracle::Schedule" << rac->GetUri();
        //}
        //// END OF BLOCK TO REMOVE
        Execute(receiver, dynamic_cast<GetOrthancImageCommand*>(protection.release()));
        break;

      case IOracleCommand::Type_GetOrthancWebViewerJpeg:
        //// DIAGNOSTIC. PLEASE REMOVE IF IT HAS BEEN COMMITTED BY MISTAKE
        //{
        //  GetOrthancWebViewerJpegCommand* rac = dynamic_cast<GetOrthancWebViewerJpegCommand*>(protection.get());
        //  LOG(TRACE) << "WebAssemblyOracle::Schedule" << rac->GetUri();
        //}
        //// END OF BLOCK TO REMOVE
        Execute(receiver, dynamic_cast<GetOrthancWebViewerJpegCommand*>(protection.release()));
        break;          
            
      case IOracleCommand::Type_Sleep:
      {
        unsigned int timeoutMS = dynamic_cast<SleepOracleCommand*>(command)->GetDelay();
        emscripten_set_timeout(TimeoutContext::Callback, timeoutMS,
                               new TimeoutContext(*this, receiver, protection.release()));
        break;
      }
            
      default:
        LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle: " << command->GetType();
    }
  }
}
