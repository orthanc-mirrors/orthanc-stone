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
      oracle_.EmitMessage(receiver, message);
    }
  };


  class WebAssemblyOracle::FetchContext : public boost::noncopyable
  {
  private:
    Emitter                        emitter_;
    const IObserver&               receiver_;
    std::auto_ptr<IOracleCommand>  command_;
    std::string                    expectedContentType_;

  public:
    FetchContext(WebAssemblyOracle& oracle,
                 const IObserver& receiver,
                 IOracleCommand* command,
                 const std::string& expectedContentType) :
      emitter_(oracle),
      receiver_(receiver),
      command_(command),
      expectedContentType_(expectedContentType)
    {
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

    static void SuccessCallback(emscripten_fetch_t *fetch)
    {
      /**
       * Firstly, make a local copy of the fetched information, and
       * free data associated with the fetch.
       **/
      
      std::auto_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));

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
      if (!context->GetExpectedContentType().empty())
      {
        headers["Content-Type"] = context->GetExpectedContentType();
      }
      
      
      emscripten_fetch_close(fetch);


      /**
       * Secondly, use the retrieved data.
       **/

      try
      {
        if (context.get() == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
        else
        {
          switch (context->GetCommand().GetType())
          {
            case IOracleCommand::Type_OrthancRestApi:
            {
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
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Error while processing a fetch answer in the oracle: " << e.What();
      }
    }

    static void FailureCallback(emscripten_fetch_t *fetch)
    {
      std::auto_ptr<FetchContext> context(reinterpret_cast<FetchContext*>(fetch->userData));
      
      LOG(ERROR) << "Fetching " << fetch->url << " failed, HTTP failure status code: " << fetch->status;

      /**
       * TODO - The following code leads to an infinite recursion, at
       * least with Firefox running on incognito mode => WHY?
       **/      
      //emscripten_fetch_close(fetch); // Also free data on failure.
    }
  };
    


  class WebAssemblyOracle::FetchCommand : public boost::noncopyable
  {
  private:
    WebAssemblyOracle&             oracle_;
    const IObserver&               receiver_;
    std::auto_ptr<IOracleCommand>  command_;
    Orthanc::HttpMethod            method_;
    std::string                    uri_;
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

    void SetUri(const std::string& uri)
    {
      uri_ = uri;
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
      if (command_.get() == NULL)
      {
        // Cannot call Execute() twice
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

      attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
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

      if (!body_.empty())
      {
        attr.requestDataSize = body_.size();
        attr.requestData = body_.c_str();
      }

      // Must be the last call to prevent memory leak on error
      attr.userData = new FetchContext(oracle_, receiver_, command_.release(), expectedContentType);
      emscripten_fetch(&attr, uri_.c_str());
    }        
  };
    
    
  void WebAssemblyOracle::Execute(const IObserver& receiver,
                                  OrthancRestApiCommand* command)
  {
    FetchCommand fetch(*this, receiver, command);

    fetch.SetMethod(command->GetMethod());
    fetch.SetUri(command->GetUri());
    fetch.SetHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
      
    if (command->GetMethod() == Orthanc::HttpMethod_Put ||
        command->GetMethod() == Orthanc::HttpMethod_Put)
    {
      std::string body;
      command->SwapBody(body);
      fetch.SetBody(body);
    }
      
    fetch.Execute();
  }
    
    
  void WebAssemblyOracle::Execute(const IObserver& receiver,
                                  GetOrthancImageCommand* command)
  {
    FetchCommand fetch(*this, receiver, command);

    fetch.SetUri(command->GetUri());
    fetch.SetHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
      
    fetch.Execute();
  }
    
    
  void WebAssemblyOracle::Execute(const IObserver& receiver,
                                  GetOrthancWebViewerJpegCommand* command)
  {
    FetchCommand fetch(*this, receiver, command);

    fetch.SetUri(command->GetUri());
    fetch.SetHttpHeaders(command->GetHttpHeaders());
    fetch.SetTimeout(command->GetTimeout());
      
    fetch.Execute();
  }



  void WebAssemblyOracle::Schedule(const IObserver& receiver,
                                   IOracleCommand* command)
  {
    std::auto_ptr<IOracleCommand> protection(command);

    if (command == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    switch (command->GetType())
    {
      case IOracleCommand::Type_OrthancRestApi:
        Execute(receiver, dynamic_cast<OrthancRestApiCommand*>(protection.release()));
        break;
        
      case IOracleCommand::Type_GetOrthancImage:
        Execute(receiver, dynamic_cast<GetOrthancImageCommand*>(protection.release()));
        break;

      case IOracleCommand::Type_GetOrthancWebViewerJpeg:
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
