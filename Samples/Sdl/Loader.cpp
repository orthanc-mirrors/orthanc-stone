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

// From Stone
#include "../../Framework/StoneInitialization.h"
#include "../../Framework/Messages/IMessage.h"
#include "../../Framework/Messages/MessageBroker.h"
#include "../../Framework/Messages/ICallable.h"
#include "../../Framework/Messages/IObservable.h"
#include "../../Framework/Volumes/ImageBuffer3D.h"

// From Orthanc framework
#include <Core/IDynamicObject.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PngWriter.h>
#include <Core/Logging.h>
#include <Core/HttpClient.h>
#include <Core/MultiThreading/SharedMessageQueue.h>
#include <Core/OrthancException.h>

#include <json/reader.h>
#include <json/value.h>
#include <json/writer.h>

#include <list>
#include <stdio.h>



namespace Refactoring
{
  class IOracleCommand : public boost::noncopyable
  {
  public:
    enum Type
    {
      Type_OrthancApi
    };

    virtual ~IOracleCommand()
    {
    }

    virtual Type GetType() const = 0;
  };


  class IMessageEmitter : public boost::noncopyable
  {
  public:
    virtual ~IMessageEmitter()
    {
    }

    virtual void EmitMessage(const OrthancStone::IObserver& observer,
                             const OrthancStone::IMessage& message) = 0;
  };


  class IOracle : public boost::noncopyable
  {
  public:
    virtual ~IOracle()
    {
    }

    virtual void Schedule(const OrthancStone::IObserver& receiver,
                          IOracleCommand* command) = 0;  // Takes ownership
  };




  class OracleCommandWithPayload : public IOracleCommand
  {
  private:
    std::auto_ptr<Orthanc::IDynamicObject>  payload_;

  public:
    void SetPayload(Orthanc::IDynamicObject* payload)
    {
      if (payload == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      else
      {
        payload_.reset(payload);
      }    
    }

    bool HasPayload() const
    {
      return (payload_.get() != NULL);
    }

    const Orthanc::IDynamicObject& GetPayload() const
    {
      if (HasPayload())
      {
        return *payload_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }
  };



  typedef std::map<std::string, std::string>  HttpHeaders;

  class OrthancApiOracleCommand : public OracleCommandWithPayload
  {
  public:
    class SuccessMessage : public OrthancStone::OriginMessage<OrthancStone::MessageType_HttpRequestSuccess,   // TODO
                                                              OrthancApiOracleCommand>
    {
    private:
      HttpHeaders   headers_;
      std::string   answer_;

    public:
      SuccessMessage(const OrthancApiOracleCommand& command,
                     const HttpHeaders& answerHeaders,
                     std::string& answer  /* will be swapped to avoid a memcpy() */) :
        OriginMessage(command),
        headers_(answerHeaders),
        answer_(answer)
      {
      }

      const std::string& GetAnswer() const
      {
        return answer_;
      }

      void GetJsonBody(Json::Value& target) const
      {
        Json::Reader reader;
        if (!reader.parse(answer_, target))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      }

      const HttpHeaders&  GetAnswerHeaders() const
      {
        return headers_;
      }
    };


    class FailureMessage : public OrthancStone::OriginMessage<OrthancStone::MessageType_HttpRequestError,   // TODO
                                                              OrthancApiOracleCommand>
    {
    private:
      Orthanc::HttpStatus  status_;

    public:
      FailureMessage(const OrthancApiOracleCommand& command,
                     Orthanc::HttpStatus status) :
        OriginMessage(command),
        status_(status)
      {
      }

      Orthanc::HttpStatus GetHttpStatus() const
      {
        return status_;
      }
    };


  private:
    Orthanc::HttpMethod  method_;
    std::string          uri_;
    std::string          body_;
    HttpHeaders          headers_;
    unsigned int         timeout_;

    std::auto_ptr< OrthancStone::MessageHandler<SuccessMessage> >  successCallback_;
    std::auto_ptr< OrthancStone::MessageHandler<FailureMessage> >  failureCallback_;

  public:
    OrthancApiOracleCommand() :
      method_(Orthanc::HttpMethod_Get),
      uri_("/"),
      timeout_(10)
    {
    }

    virtual Type GetType() const
    {
      return Type_OrthancApi;
    }

    void SetMethod(Orthanc::HttpMethod method)
    {
      method_ = method;
    }

    void SetUri(const std::string& uri)
    {
      uri_ = uri;
    }

    void SetBody(const std::string& body)
    {
      body_ = body;
    }

    void SetBody(const Json::Value& json)
    {
      Json::FastWriter writer;
      body_ = writer.write(json);
    }

    void SetHttpHeader(const std::string& key,
                       const std::string& value)
    {
      headers_[key] = value;
    }

    Orthanc::HttpMethod GetMethod() const
    {
      return method_;
    }

    const std::string& GetUri() const
    {
      return uri_;
    }

    const std::string& GetBody() const
    {
      if (method_ == Orthanc::HttpMethod_Post ||
          method_ == Orthanc::HttpMethod_Put)
      {
        return body_;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
    }

    const HttpHeaders& GetHttpHeaders() const
    {
      return headers_;
    }

    void SetTimeout(unsigned int seconds)
    {
      timeout_ = seconds;
    }

    unsigned int GetTimeout() const
    {
      return timeout_;
    }
  };



  class NativeOracle : public IOracle
  {
  private:
    class Item : public Orthanc::IDynamicObject
    {
    private:
      const OrthancStone::IObserver&  receiver_;
      std::auto_ptr<IOracleCommand>   command_;

    public:
      Item(const OrthancStone::IObserver& receiver,
           IOracleCommand* command) :
        receiver_(receiver),
        command_(command)
      {
        if (command == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
      }

      const OrthancStone::IObserver& GetReceiver() const
      {
        return receiver_;
      }

      const IOracleCommand& GetCommand() const
      {
        assert(command_.get() != NULL);
        return *command_;
      }
    };


    enum State
    {
      State_Setup,
      State_Running,
      State_Stopped
    };


    IMessageEmitter&               emitter_;
    Orthanc::WebServiceParameters  orthanc_;
    Orthanc::SharedMessageQueue    queue_;
    State                          state_;
    boost::mutex                   mutex_;
    std::vector<boost::thread*>    workers_;


    void Execute(const OrthancStone::IObserver& receiver,
                 const OrthancApiOracleCommand& command)
    {
      Orthanc::HttpClient  client(orthanc_, command.GetUri());
      client.SetMethod(command.GetMethod());
      client.SetBody(command.GetBody());
      client.SetTimeout(command.GetTimeout());
      
      {
        const HttpHeaders& headers = command.GetHttpHeaders();
        for (HttpHeaders::const_iterator it = headers.begin(); it != headers.end(); it++ )
        {
          client.AddHeader(it->first, it->second);
        }
      }

      std::string answer;
      HttpHeaders answerHeaders;

      bool success;
      try
      {
        success = client.Apply(answer, answerHeaders);
      }
      catch (Orthanc::OrthancException& e)
      {
        success = false;
      }

      if (success)
      {
        OrthancApiOracleCommand::SuccessMessage message(command, answerHeaders, answer);
        emitter_.EmitMessage(receiver, message);
      }
      else
      {
        OrthancApiOracleCommand::FailureMessage message(command, client.GetLastStatus());
        emitter_.EmitMessage(receiver, message);
      }
    }



    void Step()
    {
      std::auto_ptr<Orthanc::IDynamicObject>  object(queue_.Dequeue(100));

      if (object.get() != NULL)
      {
        const Item& item = dynamic_cast<Item&>(*object);

        switch (item.GetCommand().GetType())
        {
          case IOracleCommand::Type_OrthancApi:
            Execute(item.GetReceiver(), 
                    dynamic_cast<const OrthancApiOracleCommand&>(item.GetCommand()));
            break;

          default:
            throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
        }
      }
    }


    static void Worker(NativeOracle* that)
    {
      assert(that != NULL);
      
      for (;;)
      {
        {
          boost::mutex::scoped_lock lock(that->mutex_);
          if (that->state_ != State_Running)
          {
            return;
          }
        }

        that->Step();
      }
    }


    void StopInternal()
    {
      {
        boost::mutex::scoped_lock lock(mutex_);

        if (state_ == State_Setup ||
            state_ == State_Stopped)
        {
          return;
        }
        else
        {
          state_ = State_Stopped;
        }
      }

      for (size_t i = 0; i < workers_.size(); i++)
      {
        if (workers_[i] != NULL)
        {
          if (workers_[i]->joinable())
          {
            workers_[i]->join();
          }

          delete workers_[i];
        }
      } 
    }


  public:
    NativeOracle(IMessageEmitter& emitter) :
    emitter_(emitter),
      state_(State_Setup),
      workers_(4)
    {
    }

    virtual ~NativeOracle()
    {
      StopInternal();
    }

    void SetOrthancParameters(const Orthanc::WebServiceParameters& orthanc)
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (state_ != State_Setup)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        orthanc_ = orthanc;
      }
    }

    void SetWorkersCount(unsigned int count)
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (count <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
      else if (state_ != State_Setup)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        workers_.resize(count);
      }
    }

    void Start()
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (state_ != State_Setup)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      else
      {
        state_ = State_Running;

        for (unsigned int i = 0; i < workers_.size(); i++)
        {
          workers_[i] = new boost::thread(Worker, this);
        }
      }      
    }

    void Stop()
    {
      StopInternal();
    }

    virtual void Schedule(const OrthancStone::IObserver& receiver,
                          IOracleCommand* command)
    {
      queue_.Enqueue(new Item(receiver, command));
    }
  };



  class NativeApplicationContext : public IMessageEmitter
  {
  private:
    boost::shared_mutex            mutex_;
    OrthancStone::MessageBroker    broker_;
    OrthancStone::IObservable      oracleObservable_;

  public:
    NativeApplicationContext() :
      oracleObservable_(broker_)
    {
    }


    virtual void EmitMessage(const OrthancStone::IObserver& observer,
                             const OrthancStone::IMessage& message)
    {
      boost::unique_lock<boost::shared_mutex>  lock(mutex_);
      oracleObservable_.EmitMessage(observer, message);
    }


    class ReaderLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::shared_lock<boost::shared_mutex>  lock_;

    public:
      ReaderLock(NativeApplicationContext& that) : 
      that_(that),
      lock_(that.mutex_)
      {
      }
    };


    class WriterLock : public boost::noncopyable
    {
    private:
      NativeApplicationContext&                that_;
      boost::unique_lock<boost::shared_mutex>  lock_;

    public:
      WriterLock(NativeApplicationContext& that) : 
      that_(that),
      lock_(that.mutex_)
      {
      }

      OrthancStone::MessageBroker& GetBroker() 
      {
        return that_.broker_;
      }

      OrthancStone::IObservable& GetOracleObservable()
      {
        return that_.oracleObservable_;
      }
    };
  };



  class AxialVolumeOrthancLoader : public OrthancStone::IObserver
  {
  private:
    void Handle(const Refactoring::OrthancApiOracleCommand::SuccessMessage& message)
    {
      Json::Value v;
      message.GetJsonBody(v);

      printf("ICI [%s]\n", v.toStyledString().c_str());
    }


    std::auto_ptr<OrthancStone::ImageBuffer3D>  image_;


  public:
    AxialVolumeOrthancLoader(OrthancStone::IObservable& oracle) :
      IObserver(oracle.GetBroker())
    {
      oracle.RegisterObserverCallback(
        new OrthancStone::Callable<AxialVolumeOrthancLoader, OrthancApiOracleCommand::SuccessMessage>
        (*this, &AxialVolumeOrthancLoader::Handle));
    }
  };

}



class Toto : public OrthancStone::IObserver
{
private:
  void Handle(const Refactoring::OrthancApiOracleCommand::SuccessMessage& message)
  {
    Json::Value v;
    message.GetJsonBody(v);

    printf("ICI [%s]\n", v.toStyledString().c_str());
  }

  void Handle(const Refactoring::OrthancApiOracleCommand::FailureMessage& message)
  {
    printf("ERROR %d\n", message.GetHttpStatus());
  }

public:
  Toto(OrthancStone::IObservable& oracle) :
    IObserver(oracle.GetBroker())
  {
    oracle.RegisterObserverCallback
      (new OrthancStone::Callable
       <Toto, Refactoring::OrthancApiOracleCommand::SuccessMessage>(*this, &Toto::Handle));
  }
};


void Run(Refactoring::NativeApplicationContext& context)
{
  std::auto_ptr<Toto> toto;

  {
    Refactoring::NativeApplicationContext::WriterLock lock(context);
    toto.reset(new Toto(lock.GetOracleObservable()));
  }

  std::auto_ptr<Refactoring::AxialVolumeOrthancLoader> loader;

  {
    Refactoring::NativeApplicationContext::WriterLock lock(context);
    loader.reset(new Refactoring::AxialVolumeOrthancLoader(lock.GetOracleObservable()));
  }

  Refactoring::NativeOracle oracle(context);

  {
    Orthanc::WebServiceParameters p;
    //p.SetUrl("http://localhost:8043/");
    p.SetCredentials("orthanc", "orthanc");
    oracle.SetOrthancParameters(p);
  }

  oracle.Start();

  {
    Json::Value v = Json::objectValue;
    v["Level"] = "Series";
    v["Query"] = Json::objectValue;

    std::auto_ptr<Refactoring::OrthancApiOracleCommand>  command(new Refactoring::OrthancApiOracleCommand);
    command->SetMethod(Orthanc::HttpMethod_Post);
    command->SetUri("/tools/find");
    command->SetBody(v);

    oracle.Schedule(*toto, command.release());
  }

  boost::this_thread::sleep(boost::posix_time::seconds(1));

  oracle.Stop();
}



/**
 * IMPORTANT: The full arguments to "main()" are needed for SDL on
 * Windows. Otherwise, one gets the linking error "undefined reference
 * to `SDL_main'". https://wiki.libsdl.org/FAQWindows
 **/
int main(int argc, char* argv[])
{
  OrthancStone::StoneInitialize();
  Orthanc::Logging::EnableInfoLevel(true);

  try
  {
    Refactoring::NativeApplicationContext context;
    Run(context);
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "EXCEPTION: " << e.What();
  }

  OrthancStone::StoneFinalize();

  return 0;
}
