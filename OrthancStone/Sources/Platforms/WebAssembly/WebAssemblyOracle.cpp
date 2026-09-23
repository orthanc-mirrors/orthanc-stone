/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#if defined(ORTHANC_BUILDING_STONE_LIBRARY) && ORTHANC_BUILDING_STONE_LIBRARY == 1
#  include "WebAssemblyOracle_Includes.h"
#else
// This is the case when using the WebAssembly side module, and this
// source file must be compiled within the WebAssembly main module
#  include <Oracle/WebAssemblyOracle_Includes.h>
#endif

#include "../../Oracle/OracleCallback.h"
#include "../../Toolbox/StoneToolbox.h"

#include <OrthancException.h>
#include <Toolbox.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>


#if ORTHANC_ENABLE_DCMTK == 1
static unsigned int BUCKET_SOP = 1;
#endif


namespace OrthancStone
{
  static void TimeoutCallback(void *userData)
  {
    std::unique_ptr<IOracleCallback> callback(reinterpret_cast<IOracleCallback*>(userData));

    const SleepOracleCommand& command = dynamic_cast<const SleepOracleCommand&>(callback->GetCommand());  // TODO Refactoring - Remove this
    callback->NotifySuccess(new SleepOracleCommand::TimeoutMessage(command));
  }


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
    WebAssemblyOracle&                oracle_;  // TODO Refactoring - Remove this
    std::unique_ptr<IOracleCallback>  callback_;
    std::string                       expectedContentType_;

  public:
    FetchContext(WebAssemblyOracle& oracle,
                 IOracleCallback* callback /* takes ownership */,
                 const std::string& expectedContentType) :
      callback_(callback),
      oracle_(oracle),
      expectedContentType_(expectedContentType)
    {
      if (callback == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    const std::string& GetExpectedContentType() const
    {
      return expectedContentType_;
    }

    void EmitException(const Orthanc::OrthancException& exception)
    {
      callback_->NotifyError(exception);
    }

    void ProcessFetchResult(const std::string& answer,
                            const HttpHeaders& headers)
    {
      oracle_.ProcessFetchResult(*callback_, answer, headers);
    }

    static void SuccessCallback(emscripten_fetch_t *fetch)
    {
      /**
       * Firstly, make a local copy of the fetched information, and
       * free data associated with the fetch.
       **/
      
      if (fetch->userData == NULL)
      {
        LOG(ERROR) << "WebAssemblyOracle::FetchContext::SuccessCallback fetch->userData is NULL!";
        return;
      }

      std::unique_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));

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
          context->ProcessFetchResult(answer, headers);
        }
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(INFO) << "Error while processing a fetch answer in the oracle: " << e.What();
        context->EmitException(e);
      }
    }

    static void FailureCallback(emscripten_fetch_t *fetch)
    {
      std::unique_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));

#if 0
      {
        const size_t kEmscriptenStatusTextSize = sizeof(emscripten_fetch_t::statusText);
        char message[kEmscriptenStatusTextSize + 1];
        memcpy(message, fetch->statusText, kEmscriptenStatusTextSize);
        message[kEmscriptenStatusTextSize] = 0;

        LOG(ERROR) << "Fetching " << fetch->url
                   << " failed, HTTP failure status code: " << fetch->status
                   << " | statusText = " << message
                   << " | numBytes = " << fetch->numBytes
                   << " | totalBytes = " << fetch->totalBytes
                   << " | readyState = " << fetch->readyState;
      }
#endif

      context->EmitException(Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol));
      
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
    WebAssemblyOracle&                oracle_;  // TODO Refactoring - Remove this
    std::unique_ptr<IOracleCallback>  callback_;
    Orthanc::HttpMethod            method_;
    std::string                    url_;
    size_t                         bodySize_;
    void*                          bodyData_;
    HttpHeaders                    headers_;
    unsigned int                   timeout_;
    std::string                    expectedContentType_;
    bool                           hasCredentials_;
    std::string                    username_;
    std::string                    password_;

    void ClearBody()
    {
      if (bodyData_ != NULL)
      {
        free(bodyData_);
        bodySize_ = 0;
        bodyData_ = NULL;
      }
    }

  public:
    FetchCommand(WebAssemblyOracle& oracle,
                 IOracleCallback* callback) :
      oracle_(oracle),
      callback_(callback),
      method_(Orthanc::HttpMethod_Get),
      bodySize_(0),
      bodyData_(NULL),
      timeout_(0),
      hasCredentials_(false)
    {
      if (callback == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    ~FetchCommand()
    {
      ClearBody();
    }

    const IOracleCallback& GetCallback() const
    {
      if (callback_.get() == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        return *callback_;
      }
    }

    void SetMethod(Orthanc::HttpMethod method)
    {
      method_ = method;
    }

    Orthanc::HttpMethod GetMethod() const
    {
      return method_;
    }

    void SetUrl(const std::string& url)
    {
      url_ = url;
    }

    const std::string& GetUrl() const
    {
      return url_;
    }

    void SetBody(const std::string& body)
    {
      ClearBody();

      if (!body.empty())
      {
        bodyData_ = malloc(body.size());
        if (bodyData_ == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotEnoughMemory);
        }

        bodySize_ = body.size();
        memcpy(bodyData_, body.c_str(), body.size());
      }
    }

    void AddHttpHeaders(const HttpHeaders& headers)
    {
      for (HttpHeaders::const_iterator it = headers.begin(); it != headers.end(); ++it)
      {
        headers_[it->first] = it->second;
      }
    }

    const HttpHeaders& GetHttpHeaders() const
    {
      return headers_;
    }

    void SetTimeout(unsigned int timeout)
    {
      timeout_ = timeout;
    }

    unsigned int GetTimeout() const
    {
      return timeout_;
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
      if (callback_.get() == NULL)
      {
        // Cannot call Execute() twice
        LOG(ERROR) << "WebAssemblyOracle::Execute(): (callback_.get() == NULL)";
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

      attr.userData = new FetchContext(oracle_, callback_.release(), expectedContentType);

      attr.requestHeaders = &headers[0];
      attr.requestDataSize = bodySize_;
      attr.requestData = reinterpret_cast<const char*>(bodyData_);

      // Detach the body, it will be freed by emscripten_fetch()
      bodySize_ = NULL;
      bodyData_ = NULL;

      // Must be the last call to prevent memory leak on error
      emscripten_fetch(&attr, url_.c_str());
    }
  };


  void WebAssemblyOracle::ProcessFetchResult(IOracleCallback& callback,
                                             const std::string& answer,
                                             const HttpHeaders& headers)
  {
    switch (callback.GetCommand().GetType())
    {
      case IOracleCommand::Type_Http:
      {
        callback.NotifySuccess(new HttpCommand::SuccessMessage(
                                 dynamic_cast<const HttpCommand&>(callback.GetCommand()), headers, answer));
        break;
      }

      case IOracleCommand::Type_OrthancRestApi:
      {
        LOG(TRACE) << "WebAssemblyOracle::FetchContext::SuccessCallback. About to call EmitMessage(message);";
        callback.NotifySuccess(new OrthancRestApiCommand::SuccessMessage(
                                 dynamic_cast<const OrthancRestApiCommand&>(callback.GetCommand()), headers, answer));
        break;
      }

      case IOracleCommand::Type_GetOrthancImage:
      {
        dynamic_cast<const GetOrthancImageCommand&>(callback.GetCommand()).ProcessHttpAnswer(callback, answer, headers);
        break;
      }

      case IOracleCommand::Type_GetOrthancWebViewerJpeg:
      {
        dynamic_cast<const GetOrthancWebViewerJpegCommand&>(callback.GetCommand()).ProcessHttpAnswer(callback, answer);
        break;
      }

      case IOracleCommand::Type_ParseDicomFromWado:
      {
#if ORTHANC_ENABLE_DCMTK == 1
        const ParseDicomFromWadoCommand& c = dynamic_cast<const ParseDicomFromWadoCommand&>(callback.GetCommand());
              
        size_t fileSize;
        std::unique_ptr<Orthanc::ParsedDicomFile> dicom
          (ParseDicomSuccessMessage::ParseWadoAnswer(fileSize, answer, headers));

        callback.NotifySuccess(new ParseDicomSuccessMessage(c, c.GetSource(), *dicom, fileSize, true));

        if (dicomCache_.get())
        {
          // Store it into the cache for future use
          dicomCache_->Acquire(BUCKET_SOP, c.GetSopInstanceUid(), dicom.release(), fileSize, true);
        }
#else
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
#endif
        break;
      }

      default:
        LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle (in SuccessCallback): "
                   << callback.GetCommand().GetType();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }


  void WebAssemblyOracle::SetOrthancUrl(FetchCommand& command,
                                        const std::string& uri) const
  {
    if (isLocalOrthanc_)
    {
      command.SetUrl(StoneToolbox::JoinUrl(localOrthancRoot_, uri));
    }
    else
    {
      command.SetUrl(StoneToolbox::JoinUrl(remoteOrthanc_.GetUrl(), uri));
      command.AddHttpHeaders(remoteOrthanc_.GetHttpHeaders());
      
      if (!remoteOrthanc_.GetUsername().empty())
      {
        command.SetCredentials(remoteOrthanc_.GetUsername(), remoteOrthanc_.GetPassword());
      }
    }
  }
    

  void WebAssemblyOracle::ExecuteHttpCommand(FetchCommand& fetch)
  {
    const HttpCommand& command = dynamic_cast<const HttpCommand&>(fetch.GetCallback().GetCommand());

    fetch.SetMethod(command.GetMethod());
    fetch.SetUrl(command.GetUrl());
    fetch.AddHttpHeaders(command.GetHttpHeaders());
    fetch.SetTimeout(command.GetTimeout());

    if (command.GetMethod() == Orthanc::HttpMethod_Post ||
        command.GetMethod() == Orthanc::HttpMethod_Put)
    {
      fetch.SetBody(command.GetBody());
    }
    
    fetch.Execute();
  }
  

  void WebAssemblyOracle::ExecuteOrthancRestApiCommand(FetchCommand& fetch)
  {
    const OrthancRestApiCommand& command = dynamic_cast<const OrthancRestApiCommand&>(fetch.GetCallback().GetCommand());

    try
    {
      //LOG(TRACE) << "*********** WebAssemblyOracle::Execute.";
      //LOG(TRACE) << "WebAssemblyOracle::Execute | command = " << command;

      fetch.SetMethod(command.GetMethod());
      SetOrthancUrl(fetch, command.GetUri());
      fetch.AddHttpHeaders(command.GetHttpHeaders());
      fetch.SetTimeout(command.GetTimeout());

      if (command.GetMethod() == Orthanc::HttpMethod_Post ||
          command.GetMethod() == Orthanc::HttpMethod_Put)
      {
        fetch.SetBody(command.GetBody());
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
    
    
  void WebAssemblyOracle::ExecuteGetOrthancImageCommand(FetchCommand& fetch)
  {
    const GetOrthancImageCommand& command = dynamic_cast<const GetOrthancImageCommand&>(fetch.GetCallback().GetCommand());

    SetOrthancUrl(fetch, command.GetUri());
    fetch.AddHttpHeaders(command.GetHttpHeaders());
    fetch.SetTimeout(command.GetTimeout());
      
    fetch.Execute();
  }
    
    
  void WebAssemblyOracle::ExecuteGetOrthancWebViewerJpegCommand(FetchCommand& fetch)
  {
    const GetOrthancWebViewerJpegCommand& command = dynamic_cast<const GetOrthancWebViewerJpegCommand&>(fetch.GetCallback().GetCommand());

    SetOrthancUrl(fetch, command.GetUri());
    fetch.AddHttpHeaders(command.GetHttpHeaders());
    fetch.SetTimeout(command.GetTimeout());
      
    fetch.Execute();
  }


  void WebAssemblyOracle::ExecuteParseDicomFromWadoCommand(IOracleCallback* callback)
  {
    std::unique_ptr<IOracleCallback> protection(callback);

    const ParseDicomFromWadoCommand& command = dynamic_cast<const ParseDicomFromWadoCommand&>(protection->GetCommand());

#if ORTHANC_ENABLE_DCMTK == 1
    if (dicomCache_.get())
    {
      ParsedDicomCache::Reader reader(*dicomCache_, BUCKET_SOP, command.GetSopInstanceUid());
      if (reader.IsValid() &&
          reader.HasPixelData())
      {
        // Reuse the DICOM file from the cache
        protection->NotifySuccess(new ParseDicomSuccessMessage(command, command.GetSource(), reader.GetDicom(),
                                                               reader.GetFileSize(), reader.HasPixelData()));
        return;
      }
    }
#endif

    switch (command.GetRestCommand().GetType())
    {
      case IOracleCommand::Type_Http:
      {
        const HttpCommand& rest =
          dynamic_cast<const HttpCommand&>(command.GetRestCommand());
        
        FetchCommand fetch(*this, protection.release());
    
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
          dynamic_cast<const OrthancRestApiCommand&>(command.GetRestCommand());
        
        FetchCommand fetch(*this, protection.release());

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


  void WebAssemblyOracle::Submit(IOracleCallback* callback)
  {
    std::unique_ptr<IOracleCallback> protection(callback);

    if (callback == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    switch (protection->GetCommand().GetType())
    {
      case IOracleCommand::Type_Http:
      {
        FetchCommand fetch(*this, protection.release());
        ExecuteHttpCommand(fetch);
        break;
      }
        
      case IOracleCommand::Type_OrthancRestApi:
      {
        FetchCommand fetch(*this, protection.release());
        ExecuteOrthancRestApiCommand(fetch);
        break;
      }
        
      case IOracleCommand::Type_GetOrthancImage:
      {
        FetchCommand fetch(*this, protection.release());
        ExecuteGetOrthancImageCommand(fetch);
        break;
      }

      case IOracleCommand::Type_GetOrthancWebViewerJpeg:
      {
        FetchCommand fetch(*this, protection.release());
        ExecuteGetOrthancWebViewerJpegCommand(fetch);
        break;
      }
            
      case IOracleCommand::Type_Sleep:
      {
        unsigned int timeoutMS = dynamic_cast<const SleepOracleCommand&>(protection->GetCommand()).GetDelay();
        emscripten_set_timeout(TimeoutCallback, timeoutMS, protection.release());
        break;
      }
            
      case IOracleCommand::Type_ParseDicomFromWado:
#if ORTHANC_ENABLE_DCMTK == 1
        ExecuteParseDicomFromWadoCommand(protection.release());
#else
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                        "DCMTK must be enabled to parse DICOM files");
#endif
        break;
            
      default:
        LOG(ERROR) << "Command type not implemented by the WebAssembly Oracle (in Schedule): "
                   << protection->GetCommand().GetType();
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }


  bool WebAssemblyOracle::Schedule(boost::shared_ptr<IObserver> receiver,
                                   IOracleCommand* command)
  {
    LOG(TRACE) << "WebAssemblyOracle::Schedule : receiver = "
               << std::hex << receiver.get();

    Submit(new OldOracleCallback(command, receiver, *this));

    return true;
  }


  void WebAssemblyOracle::Submit(IEnvironment& environment,
                                 const boost::shared_ptr<IOracleClient>& client,
                                 IOracleCommand* command /* takes ownership */)
  {
    Submit(new OracleCallback(environment, client, command));
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

  
  WebAssemblyOracle::CachedInstanceAccessor::CachedInstanceAccessor(WebAssemblyOracle& oracle,
                                                                    const std::string& sopInstanceUid)
  {
#if ORTHANC_ENABLE_DCMTK == 1
    if (oracle.dicomCache_.get() != NULL)
    {
      reader_.reset(new ParsedDicomCache::Reader(*oracle.dicomCache_, BUCKET_SOP, sopInstanceUid));
    }
#endif
  }

  bool WebAssemblyOracle::CachedInstanceAccessor::IsValid() const
  {
#if ORTHANC_ENABLE_DCMTK == 1
    return (reader_.get() != NULL &&
            reader_->IsValid());
#else
    return false;
#endif
  }

#if ORTHANC_ENABLE_DCMTK == 1
  const Orthanc::ParsedDicomFile& WebAssemblyOracle::CachedInstanceAccessor::GetDicom() const
  {
    if (IsValid())
    {
      assert(reader_.get() != NULL);
      return reader_->GetDicom();
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
#endif

  size_t WebAssemblyOracle::CachedInstanceAccessor::GetFileSize() const
  {
#if ORTHANC_ENABLE_DCMTK == 1
    if (IsValid())
    {
      assert(reader_.get() != NULL);
      return reader_->GetFileSize();
    }
    else
#endif
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

  bool WebAssemblyOracle::CachedInstanceAccessor::HasPixelData() const
  {
#if ORTHANC_ENABLE_DCMTK == 1
    if (IsValid())
    {
      assert(reader_.get() != NULL);
      return reader_->HasPixelData();
    }
    else
#endif
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
}
