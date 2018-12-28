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


#include "Oracle.h"

#include <Core/Logging.h>
#include <Core/MultiThreading/SharedMessageQueue.h>
#include <Core/OrthancException.h>

#include <vector>
#include <stdio.h>
#include <boost/thread/mutex.hpp>

namespace OrthancStone
{
  class Oracle::PImpl
  {
  private:
    enum State
    {
      State_Init,
      State_Started,
      State_Stopped
    };

    boost::mutex                   oracleMutex_;
    State                          state_;
    std::vector<boost::thread*>    threads_;
    Orthanc::SharedMessageQueue    queue_;

    static void Worker(PImpl* that)
    {
      for (;;)
      {
        State state;
        
        {
          boost::mutex::scoped_lock lock(that->oracleMutex_);
          state = that->state_;
        }

        if (state == State_Stopped)
        {
          break;
        }

        std::auto_ptr<Orthanc::IDynamicObject> item(that->queue_.Dequeue(100));
        if (item.get() != NULL)
        {
          IOracleCommand& command = dynamic_cast<IOracleCommand&>(*item);
          try
          {
            command.Execute();
          }
          catch (Orthanc::OrthancException& ex)
          {
            // this is probably a curl error that has been triggered.  We may just ignore it.
            // The command.success_ will stay at false and this will be handled in the command.Commit
          }

          // Random sleeping to test
          //boost::this_thread::sleep(boost::posix_time::milliseconds(50 * (1 + rand() % 10)));

          command.Commit();
        }
      }
    }
    
  public:
    PImpl(unsigned int threadCount) :
      state_(State_Init),
      threads_(threadCount)
    {
    }

    ~PImpl()
    {
      if (state_ == State_Started)
      {
        LOG(ERROR) << "You should have manually called Oracle::Stop()";
        Stop();
      }
    }

    Orthanc::SharedMessageQueue& GetQueue()
    {
      return queue_;
    }

    void Submit(IOracleCommand* command)
    {
      std::auto_ptr<IOracleCommand> protection(command);

      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
      
      boost::mutex::scoped_lock lock(oracleMutex_);

      switch (state_)
      {
        case State_Init:
        case State_Started:
          queue_.Enqueue(protection.release());
          break;

        case State_Stopped:
          LOG(ERROR) << "Cannot schedule a request to the Oracle after having "
                     << "called Oracle::Stop()";
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      
    }

    void Start()
    {
      boost::mutex::scoped_lock lock(oracleMutex_);

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
        boost::mutex::scoped_lock lock(oracleMutex_);

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
  

  Oracle::Oracle(unsigned int threadCount) :
    pimpl_(new PImpl(threadCount))
  {
  }

  void Oracle::Start()
  {
    pimpl_->Start();
  }


  void Oracle::Submit(IOracleCommand* command)
  {
    pimpl_->Submit(command);
  }
     
   
  void Oracle::Stop()
  {
    pimpl_->Stop();
  }


  void Oracle::WaitEmpty()
  {
    pimpl_->GetQueue().WaitEmpty(50);
  }
}
