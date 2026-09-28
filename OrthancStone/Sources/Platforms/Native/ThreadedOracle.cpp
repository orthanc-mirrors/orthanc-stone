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


#include "ThreadedOracle.h"

#include "../../Oracle/OracleCallback.h"
#include "../../Oracle/SleepOracleCommand.h"
#include "GenericOracleRunner.h"

#include <Logging.h>
#include <OrthancException.h>

namespace OrthancStone
{
  class ThreadedOracle::GenericRunnable : public Orthanc::IRunnable
  {
  private:
    StoneApplication::Configuration       configuration_;
    std::unique_ptr<GenericOracleRunner>  runner_;
    std::unique_ptr<IOracleCallback>      callback_;

  public:
    GenericRunnable(const StoneApplication::Configuration& configuration,
                    GenericOracleRunner* runner /* takes ownership */,
                    IOracleCallback* callback /* takes ownership */) :
      configuration_(configuration),
      runner_(runner),
      callback_(callback)
    {
      if (runner == NULL ||
          callback == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    virtual void Run() ORTHANC_OVERRIDE
    {
      runner_->Run(*callback_);
    }
  };


  class ThreadedOracle::SleepRunnable : public Orthanc::IRunnable
  {
  private:
    class Item : public boost::noncopyable
    {
    private:
      std::unique_ptr<IOracleCallback>  callback_;
      boost::posix_time::ptime          expiration_;

    public:
      Item(IOracleCallback* callback,
           unsigned int delay) :
        callback_(callback)
      {
        if (callback == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }

        expiration_ = (boost::posix_time::microsec_clock::local_time() +
                       boost::posix_time::milliseconds(delay));
      }

      const boost::posix_time::ptime& GetExpirationTime() const
      {
        return expiration_;
      }

      IOracleCallback& GetCallback()
      {
        return *callback_;
      }
    };

    typedef std::list<Item*>  Content;

    boost::mutex  mutex_;
    Content       content_;

  public:
    ~SleepRunnable()
    {
      for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
      {
        if (*it != NULL)
        {
          delete *it;
        }
      }
    }


    void Add(IOracleCallback* callback,
             unsigned int delay)
    {
      boost::mutex::scoped_lock lock(mutex_);
      content_.push_back(new Item(callback, delay));
    }


    // Awakes expired sleeps
    virtual void Run() ORTHANC_OVERRIDE
    {
      boost::mutex::scoped_lock lock(mutex_);

      const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

      Content  stillSleeping;

      for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
      {
        if (*it != NULL &&
            (*it)->GetExpirationTime() <= now)
        {
          const SleepOracleCommand& command = dynamic_cast<const SleepOracleCommand&>((*it)->GetCallback().GetCommand()); // TODO Refactoring - Remove this
          (*it)->GetCallback().NotifySuccess(new SleepOracleCommand::TimeoutMessage(command));
          delete *it;
          *it = NULL;
        }
        else
        {
          stillSleeping.push_back(*it);
        }
      }

      // Compact the still-sleeping commands
      content_ = stillSleeping;
    }
  };


  ThreadedOracle::ThreadedOracle(const StoneApplication::Configuration& configuration) :
    configuration_(configuration),
    sleepingThread_(new SleepRunnable, configuration.GetWorkersTimeResolution())
  {
    threadPool_.SetThreadsCount(configuration.GetOracleThreadsCount());
    threadPool_.SetDequeueTimeout(configuration.GetWorkersTimeResolution());

    if (configuration.GetDicomCacheSize() == 0)
    {
      LOG(WARNING) << "The DICOM cache is disabled";
    }
    else
    {
      LOG(INFO) << "The DICOM cache size is set to " << configuration.GetDicomCacheSize() << " bytes";
      dicomCache_.reset(new ParsedDicomCache(configuration.GetDicomCacheSize()));
    }
  }


  void ThreadedOracle::Start()
  {
    sleepingThread_.Start();
    threadPool_.Start();
  }


  void ThreadedOracle::Stop()
  {
    threadPool_.Stop();
    sleepingThread_.Stop();
  }


  void ThreadedOracle::SubmitInternal(IOracleCallback* callback /* takes ownership */)
  {
    std::unique_ptr<IOracleCallback> protection(callback);

    if (callback == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (protection->GetCommand().GetType() == IOracleCommand::Type_Sleep)
    {
      const unsigned int delay = dynamic_cast<const SleepOracleCommand&>(protection->GetCommand()).GetDelay();
      SleepRunnable& runnable = dynamic_cast<SleepRunnable&>(sleepingThread_.GetRunnable());
      runnable.Add(protection.release(), delay);
    }
    else
    {
      std::unique_ptr<GenericOracleRunner> runner(new GenericOracleRunner(configuration_));

#if ORTHANC_ENABLE_DCMTK == 1
      if (dicomCache_)
      {
        runner->SetDicomCache(dicomCache_);
      }
#endif

      threadPool_.Submit(new GenericRunnable(configuration_, runner.release(), protection.release()));
    }
  }


  void ThreadedOracle::Submit(IEnvironment& environment,
                              const boost::shared_ptr<IOracleClient>& client,
                              IOracleCommand* command /* takes ownership */)
  {
    std::unique_ptr<IOracleCommand> protection(command);

    if (!client)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    SubmitInternal(new OracleCallback(environment, client, protection.release()));
  }
}
