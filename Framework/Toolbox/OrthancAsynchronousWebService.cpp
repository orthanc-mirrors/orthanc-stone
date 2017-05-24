/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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


#include "OrthancAsynchronousWebService.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Core/MultiThreading/SharedMessageQueue.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/OrthancHttpConnection.h"

namespace OrthancStone
{
  class OrthancAsynchronousWebService::PendingRequest : public Orthanc::IDynamicObject
  {
  private:
    bool                                    isPost_;
    ICallback&                              callback_;
    std::string                             uri_;
    std::string                             body_;
    std::auto_ptr<Orthanc::IDynamicObject>  payload_;

    PendingRequest(bool isPost,
                   ICallback& callback,
                   const std::string& uri,
                   const std::string& body,
                   Orthanc::IDynamicObject* payload) :
      isPost_(isPost),
      callback_(callback),
      uri_(uri),
      body_(body),
      payload_(payload)
    {
    }
    
  public:
    static PendingRequest* CreateGetRequest(ICallback& callback,
                                            const std::string& uri,
                                            Orthanc::IDynamicObject* payload)
    {
      return new PendingRequest(false, callback, uri, "", payload);
    }

    static PendingRequest* CreatePostRequest(ICallback& callback,
                                             const std::string& uri,
                                             const std::string& body,
                                             Orthanc::IDynamicObject* payload)
    {
      return new PendingRequest(true, callback, uri, body, payload);
    }

    void Execute(OrthancPlugins::OrthancHttpConnection& connection)
    {
      if (payload_.get() == NULL)
      {
        // Execute() has already been invoked
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);        
      }

      std::string answer;
      
      try
      {
        if (isPost_)
        {
          connection.RestApiPost(answer, uri_, body_);
        }
        else
        {
          connection.RestApiGet(answer, uri_);
        }
      }
      catch (Orthanc::OrthancException&)
      {
        callback_.NotifyError(uri_, payload_.release());
        return;
      }

      callback_.NotifySuccess(uri_, answer.c_str(), answer.size(), payload_.release());
    }
  };
    
  class OrthancAsynchronousWebService::PImpl : public boost::noncopyable
  {
  private:
    enum State
    {
      State_Init,
      State_Started,
      State_Stopped
    };
    
    boost::mutex                   mutex_;
    State                          state_;
    Orthanc::WebServiceParameters  orthanc_;
    std::vector<boost::thread*>    threads_;
    Orthanc::SharedMessageQueue    queue_;

    static void Worker(PImpl* that)
    {
      OrthancPlugins::OrthancHttpConnection connection(that->orthanc_);
      
      for (;;)
      {
        State state;
        
        {
          boost::mutex::scoped_lock lock(that->mutex_);
          state = that->state_;
        }

        if (state == State_Stopped)
        {
          break;
        }

        std::auto_ptr<Orthanc::IDynamicObject> request(that->queue_.Dequeue(100));
        if (request.get() != NULL)
        {
          dynamic_cast<PendingRequest&>(*request).Execute(connection);          
        }
      }
    }
    
  public:
    PImpl(const Orthanc::WebServiceParameters& orthanc,
          unsigned int threadCount) :
      state_(State_Init),
      orthanc_(orthanc),
      threads_(threadCount)
    {
    }

    ~PImpl()
    {
      if (state_ == State_Started)
      {
        LOG(ERROR) << "You should have manually called OrthancAsynchronousWebService::Stop()";
        Stop();
      }
    }

    void Schedule(PendingRequest* request)
    {
      std::auto_ptr<PendingRequest> protection(request);

      if (request == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      
      boost::mutex::scoped_lock lock(mutex_);

      switch (state_)
      {
        case State_Init:
          LOG(ERROR) << "You must call OrthancAsynchronousWebService::Start()";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);

        case State_Started:
          queue_.Enqueue(protection.release());
          break;

        case State_Stopped:
          LOG(ERROR) << "Cannot schedule a Web request after having "
                     << "called OrthancAsynchronousWebService::Stop()";
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      
    }

    void Start()
    {
      boost::mutex::scoped_lock lock(mutex_);

      if (state_ != State_Init)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }
      
      for (size_t i = 0; i < threads_.size(); i++)
      {
        threads_[i] = new boost::thread(Worker, this);
      }

      state_ = State_Started;
    }

    void Stop()
    {
      {
        boost::mutex::scoped_lock lock(mutex_);

        if (state_ != State_Started)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }

        state_ = State_Stopped;
      }
      
      for (size_t i = 0; i < threads_.size(); i++)
      {
        if (threads_[i] != NULL)
        {
          if (threads_[i]->joinable())
          {
            threads_[i]->join();
          }

          delete threads_[i];
        }
      }
    }
  };

  OrthancAsynchronousWebService::OrthancAsynchronousWebService(
    const Orthanc::WebServiceParameters& parameters,
    unsigned int threadCount) :
    pimpl_(new PImpl(parameters, threadCount))
  {
  }

  void  OrthancAsynchronousWebService::ScheduleGetRequest(ICallback& callback,
                                                          const std::string& uri,
                                                          Orthanc::IDynamicObject* payload)
  {
    pimpl_->Schedule(PendingRequest::CreateGetRequest(callback, uri, payload));
  }
  

  void  OrthancAsynchronousWebService::SchedulePostRequest(ICallback& callback,
                                                           const std::string& uri,
                                                           const std::string& body,
                                                           Orthanc::IDynamicObject* payload)
  {
    pimpl_->Schedule(PendingRequest::CreatePostRequest(callback, uri, body, payload));
  }

  void  OrthancAsynchronousWebService::Start()
  {
    pimpl_->Start();
  }

  void  OrthancAsynchronousWebService::Stop()
  {
    pimpl_->Stop();
  }
}
