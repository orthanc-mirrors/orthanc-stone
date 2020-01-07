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

#include "OracleCommandExceptionMessage.h"
#include "SleepOracleCommand.h"

#if ORTHANC_ENABLE_DCMTK == 1
#  include "ParseDicomSuccessMessage.h"
static unsigned int BUCKET_SOP = 1;
#endif

#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>

namespace OrthancStone
{
  class WebAssemblyOracle::TimeoutContext
  {
  private:
    WebAssemblyOracle&                 oracle_;
    boost::weak_ptr<IObserver>         receiver_;
    std::auto_ptr<SleepOracleCommand>  command_;

  public:
    TimeoutContext(WebAssemblyOracle& oracle,
                   boost::weak_ptr<IObserver> receiver,
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
      assert(command_.get() != NULL);

      SleepOracleCommand::TimeoutMessage message(*command_);
      oracle_.EmitMessage(receiver_, message);
    }

    static void Callback(void *userData)
    {
      std::auto_ptr<TimeoutContext> context(reinterpret_cast<TimeoutContext*>(userData));
      context->EmitMessage();
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
    WebAssemblyOracle&             oracle_;
    boost::weak_ptr<IObserver>     receiver_;
    std::auto_ptr<IOracleCommand>  command_;
    std::string                    expectedContentType_;

  public:
    FetchContext(WebAssemblyOracle& oracle,
                 boost::weak_ptr<IObserver> receiver,
                 IOracleCommand* command,
                 const std::string& expectedContentType) :
      oracle_(oracle),
      receiver_(receiver),
      command_(command),
      expectedContentType_(expectedContentType)
    {
      LOG(TRACE) << "WebAssemblyOracle::FetchContext::FetchContext() | "
                 << "receiver address = " << std::hex << &receiver;

      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    const std::string& GetExpectedContentType() const
    {
      return expectedContentType_;
    }

    IMessageEmitter& GetEmitter() const
    {
      return oracle_;
    }

    boost::weak_ptr<IObserver> GetReceiver() const
    {
      return receiver_;
    }

    void EmitMessage(const IMessage& message)
    {
      LOG(TRACE) << "WebAssemblyOracle::FetchContext::EmitMessage receiver_ = "
        << std::hex << &receiver_ << std::dec;
      oracle_.EmitMessage(receiver_, message);
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

#if ORTHANC_ENABLE_DCMTK == 1
    void StoreInCache(const std::string& sopInstanceUid,
                      std::auto_ptr<Orthanc::ParsedDicomFile>& dicom,
                      size_t fileSize)
    {
      if (oracle_.dicomCache_.get())
      {
        // Store it into the cache for future use
        oracle_.dicomCache_->Acquire(BUCKET_SOP, sopInstanceUid,
                                     dicom.release(), fileSize, true);
      }              
    }
#endif

    static void SuccessCallback(emscripten_fetch_t *fetch)
    {
      /**
       * Firstly, make a local copy of the fetched information, and
       * free data associated with the fetch.
       **/
      
      if (fetch->userData == NULL)
      {
        LOG(ERROR) << "WebAssemblyOracle::FetchContext::SuccessCallback fetch->userData is NULL!!!!!!!";
        return;
      }

      std::auto_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));

      std::string answer;
      if (fetch->numBytes > 0)
      {
        answer.assign(fetch->data, fetch->numBytes);
      }


      /**
       * Retrieving the headers of the HTTP answer.
       **/
      HttpHeaders headers;

#if (__EMSCRIPTEN_major__ < 1 ||                                        \
     (__EMSCRIPTEN_major__ == 1 && __EMSCRIPTEN_minor__ < 38) ||        \
     (__EMSCRIPTEN_major__ == 1 && __EMSCRIPTEN_minor__ == 38 && __EMSCRIPTEN_tiny__ < 37))
#  warning Consider upgrading Emscripten to a version above 1.38.37, incomplete support of Fetch API

      /**
       * HACK - If emscripten < 1.38.37, the fetch API does not
       * contain a way to retrieve the HTTP headers of the answer. We
       * make the assumption that the "Content-Type" header of the
       * response is the same as the "Accept" header of the
       * query. This is fixed thanks to the
       * "emscripten_fetch_get_response_headers()" function that was
       * added to "fetch.h" at emscripten-1.38.37 on 2019-06-26.
       *
       * https://github.com/emscripten-core/emscripten/blob/1.38.37/system/include/emscripten/fetch.h
       * https://github.com/emscripten-core/emscripten/pull/8486
       **/
      if (fetch->userData != NULL)
      {
        if (!context->GetExpectedContentType().empty())
        {
          headers["Content-Type"] = context->GetExpectedContentType();
        }
      }
#else
      {
        size_t size = emscripten_fetch_get_response_headers_length(fetch);

        std::string plainHeaders(size + 1, '\0');
        emscripten_fetch_get_response_headers(fetch, &plainHeaders[0], size + 1);

        std::vector<std::string> tokens;
        Orthanc::Toolbox::TokenizeString(tokens, plainHeaders, '\n');

        for (size_t i = 0; i < tokens.size(); i++)
        {
          size_t p = tokens[i].find(':');
          if (p != std::string::npos)
          {
            std::string key = Orthanc::Toolbox::StripSpaces(tokens[i].substr(0, p));
            std::string value = Orthanc::Toolbox::StripSpaces(tokens[i].substr(p + 1));
            headers[key] = value;
          }
        }
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
                (context->GetReceiver(), context->GetEmitter(), answer, headers);
              break;
            }

            case IOracleCommand::Type_GetOrthancWebViewerJpeg:
            {
              context->GetTypedCommand<GetOrthancWebViewerJpegCommand>().ProcessHttpAnswer
                (context->GetReceiver(), context->GetEmitter(), answer);
              break;
            }

            case IOracleCommand::Type_ParseDicomFromWado:
            {
#if ORTHANC_ENABLE_DCMTK == 1
              const ParseDicomFromWadoCommand& command =
                context->GetTypedCommand<ParseDicomFromWadoCommand>();
              
              size_t fileSize;
              std::auto_ptr<Orthanc::ParsedDicomFile> dicom
                (ParseDicomSuccessMessage::ParseWadoAnswer(fileSize, answer, headers));

              {
                ParseDicomSuccessMessage message(command, *dicom, fileSize, true);
                context->EmitMessage(message);
              }

              context->StoreInCache(command.GetSopInstanceUid(), dicom, fileSize);
#else
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
#endif
              break;
            }

            default:
              LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle (in SuccessCallback): "
                         << context->GetCommand().GetType();
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
    boost::weak_ptr<IObserver>     receiver_;
    std::auto_ptr<IOracleCommand>  command_;
    Orthanc::HttpMethod            method_;
    std::string                    url_;
    std::string                    body_;
    HttpHeaders                    headers_;
    unsigned int                   timeout_;
    std::string                    expectedContentType_;
    bool                           hasCredentials_;
    std::string                    username_;
    std::string                    password_;

  public:
    FetchCommand(WebAssemblyOracle& oracle,
                 boost::weak_ptr<IObserver> receiver,
                 IOracleCommand* command) :
      oracle_(oracle),
      receiver_(receiver),
      command_(command),
      method_(Orthanc::HttpMethod_Get),
      timeout_(0),
      hasCredentials_(false)
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

    void SetUrl(const std::string& url)
    {
      url_ = url;
    }

    void SetBody(std::string& body /* will be swapped */)
    {
      body_.swap(body);
    }

    void AddHttpHeaders(const HttpHeaders& headers)
    {
      for (HttpHeaders::const_iterator it = headers.begin(); it != headers.end(); ++it)
      {
        headers_[it->first] = it->second;
      }
    }

    void SetTimeout(unsigned int timeout)
    {
      timeout_ = timeout;
    }

    void SetCredentials(const std::string& username,
                        const std::string& password)
    {
      hasCredentials_ = true;
      username_ = username;
      password_ = password;
    }

    void Execute()
    {
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

      if (hasCredentials_)
      {
        attr.withCredentials = EM_TRUE;
        attr.userName = username_.c_str();
        attr.password = password_.c_str();
      }
      
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


  void WebAssemblyOracle::SetOrthancUrl(FetchCommand& command,
                                        const std::string& uri) const
  {
    if (isLocalOrthanc_)
    {
      command.SetUrl(localOrthancRoot_ + uri);
    }
    else
    {
      command.SetUrl(remoteOrthanc_.GetUrl() + uri);
      command.AddHttpHeaders(remoteOrthanc_.GetHttpHeaders());
      
      if (!remoteOrthanc_.GetUsername().empty())
      {
        command.SetCredentials(remoteOrthanc_.GetUsername(), remoteOrthanc_.GetPassword());
      }
    }
  }
    

  void WebAssemblyOracle::Execute(boost::weak_ptr<IObserver> receiver,
                                  HttpCommand* command)
  {
    FetchCommand fetch(*this, receiver, command);
    
    fetch.SetMethod(command->GetMethod());
    fetch.SetUrl(command->GetUrl());
    fetch.AddHttpHeaders(command->GetHttpHeaders());
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
  

  void WebAssemblyOracle::Execute(boost::weak_ptr<IObserver> receiver,
                                  OrthancRestApiCommand* command)
  {
    try
    {
      //LOG(TRACE) << "*********** WebAssemblyOracle::Execute.";
      //LOG(TRACE) << "WebAssemblyOracle::Execute | command = " << command;
      FetchCommand fetch(*this, receiver, command);

      fetch.SetMethod(command->GetMethod());
      SetOrthancUrl(fetch, command->GetUri());
      fetch.AddHttpHeaders(command->GetHttpHeaders());
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
    
    
  void WebAssemblyOracle::Execute(boost::weak_ptr<IObserver> receiver,
                                  GetOrthancImageCommand* command)
  {
    FetchCommand fetch(*this, receiver, command);

    SetOrthancUrl(fetch, command->GetUri());
    fetch.AddHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
      
    fetch.Execute();
  }
    
    
  void WebAssemblyOracle::Execute(boost::weak_ptr<IObserver> receiver,
                                  GetOrthancWebViewerJpegCommand* command)
  {
    FetchCommand fetch(*this, receiver, command);

    SetOrthancUrl(fetch, command->GetUri());
    fetch.AddHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
      
    fetch.Execute();
  }


  void WebAssemblyOracle::Execute(boost::weak_ptr<IObserver> receiver,
                                  ParseDicomFromWadoCommand* command)
  {
    std::auto_ptr<ParseDicomFromWadoCommand> protection(command);
    
#if ORTHANC_ENABLE_DCMTK == 1
    if (dicomCache_.get())
    {
      ParsedDicomCache::Reader reader(*dicomCache_, BUCKET_SOP, protection->GetSopInstanceUid());
      if (reader.IsValid() &&
          reader.HasPixelData())
      {
        // Reuse the DICOM file from the cache
        ParseDicomSuccessMessage message(*protection, reader.GetDicom(),
                                         reader.GetFileSize(), reader.HasPixelData());
        EmitMessage(receiver, message);
        return;
      }
    }
#endif

    switch (command->GetRestCommand().GetType())
    {
      case IOracleCommand::Type_Http:
      {
        const HttpCommand& rest =
          dynamic_cast<const HttpCommand&>(protection->GetRestCommand());
        
        FetchCommand fetch(*this, receiver, protection.release());
    
        fetch.SetMethod(rest.GetMethod());
        fetch.SetUrl(rest.GetUrl());
        fetch.AddHttpHeaders(rest.GetHttpHeaders());
        fetch.SetTimeout(rest.GetTimeout());
    
        if (rest.GetMethod() == Orthanc::HttpMethod_Post ||
            rest.GetMethod() == Orthanc::HttpMethod_Put)
        {
          std::string body = rest.GetBody();
          fetch.SetBody(body);
        }
    
        fetch.Execute();
        break;
      }

      case IOracleCommand::Type_OrthancRestApi:
      {
        const OrthancRestApiCommand& rest =
          dynamic_cast<const OrthancRestApiCommand&>(protection->GetRestCommand());
        
        FetchCommand fetch(*this, receiver, protection.release());

        fetch.SetMethod(rest.GetMethod());
        SetOrthancUrl(fetch, rest.GetUri());
        fetch.AddHttpHeaders(rest.GetHttpHeaders());
        fetch.SetTimeout(rest.GetTimeout());

        if (rest.GetMethod() == Orthanc::HttpMethod_Post ||
            rest.GetMethod() == Orthanc::HttpMethod_Put)
        {
          std::string body = rest.GetBody();
          fetch.SetBody(body);
        }

        fetch.Execute();
        break;
      }

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }


  bool WebAssemblyOracle::Schedule(boost::shared_ptr<IObserver> receiver,
                                   IOracleCommand* command)
  {
    LOG(TRACE) << "WebAssemblyOracle::Schedule : receiver = "
               << std::hex << &receiver;
    
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
        Execute(receiver, dynamic_cast<OrthancRestApiCommand*>(protection.release()));
        break;
        
      case IOracleCommand::Type_GetOrthancImage:
        Execute(receiver, dynamic_cast<GetOrthancImageCommand*>(protection.release()));
        break;

      case IOracleCommand::Type_GetOrthancWebViewerJpeg:
        break;          
            
      case IOracleCommand::Type_Sleep:
      {
        unsigned int timeoutMS = dynamic_cast<SleepOracleCommand*>(command)->GetDelay();
        emscripten_set_timeout(TimeoutContext::Callback, timeoutMS,
                               new TimeoutContext(*this, receiver, protection.release()));
        break;
      }
            
      case IOracleCommand::Type_ParseDicomFromWado:
#if ORTHANC_ENABLE_DCMTK == 1
        Execute(receiver, dynamic_cast<ParseDicomFromWadoCommand*>(protection.release()));
#else
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                          "DCMTK must be enabled to parse DICOM files");
#endif
        break;
            
      default:
        LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle (in Schedule): "
                   << command->GetType();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    return true;
  }


  void WebAssemblyOracle::SetDicomCacheSize(size_t size)
  {
#if ORTHANC_ENABLE_DCMTK == 1
    if (size == 0)
    {
      dicomCache_.reset();
    }
    else
    {
      dicomCache_.reset(new ParsedDicomCache(size));
    }
#else
    LOG(INFO) << "DCMTK support is disabled, the DICOM cache is disabled";
#endif
  }
}
