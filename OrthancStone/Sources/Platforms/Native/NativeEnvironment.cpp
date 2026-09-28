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


#include "NativeEnvironment.h"

#include "../../Oracle/OracleCallback.h"


namespace OrthancStone
{
  class NativeEnvironment::Notification : public Orthanc::IDynamicObject
  {
  private:
    NativeEnvironment&               environment_;
    boost::weak_ptr<IOracleClient>   client_;
    std::unique_ptr<IOracleCommand>  command_;

  protected:
    virtual void NotifyInternal(IOracleClient& client,
                                const IOracleCommand& command) = 0;

  public:
    Notification(NativeEnvironment& environment,
                 const boost::weak_ptr<IOracleClient>& client,
                 IOracleCommand* command /* takes ownership */) :
      environment_(environment),
      client_(client),
      command_(command)
    {
      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    void NotifyClient()
    {
      NativeEnvironment::Lock environmentLock(environment_);

      if (!environmentLock.IsFirstLock())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
      }

      {
        boost::shared_ptr<IOracleClient> clientLock(client_.lock());

        if (clientLock)
        {
          NotifyInternal(*clientLock, *command_);
        }
      }
    }
  };


  class NativeEnvironment::SuccessNotification : public Notification
  {
  private:
    std::unique_ptr<IMessage>  result_;

  protected:
    virtual void NotifyInternal(IOracleClient& client,
                                const IOracleCommand& command) ORTHANC_OVERRIDE
    {
      client.HandleSuccessFromOracle(command, *result_);
    }

  public:
    SuccessNotification(NativeEnvironment& environment,
                        const boost::weak_ptr<IOracleClient>& client,
                        IOracleCommand* command /* takes ownership */,
                        IMessage* result /* takes ownership */) :
      Notification(environment, client, command),
      result_(result)
    {
      if (result == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }
  };


  class NativeEnvironment::ErrorNotification : public Notification
  {
  private:
    Orthanc::OrthancException  error_;

  protected:
    virtual void NotifyInternal(IOracleClient& client,
                                const IOracleCommand& command) ORTHANC_OVERRIDE
    {
      client.HandleErrorFromOracle(command, error_);
    }

  public:
    ErrorNotification(NativeEnvironment& environment,
                      const boost::weak_ptr<IOracleClient>& client,
                      IOracleCommand* command /* takes ownership */,
                      const Orthanc::OrthancException& error) :
      Notification(environment, client, command),
      error_(error)
    {
    }
  };


  class NativeEnvironment::NotificationRunnable : public Orthanc::IRunnable
  {
  private:
    Orthanc::SharedMessageQueue& queue_;
    unsigned int                 timeResolution_;

  public:
    NotificationRunnable(Orthanc::SharedMessageQueue& queue,
                         unsigned int timeResolution /* milliseconds */) :
      queue_(queue),
      timeResolution_(timeResolution)
    {
      if (timeResolution == 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    }

    virtual void Run() ORTHANC_OVERRIDE
    {
      std::unique_ptr<Orthanc::IDynamicObject> completion(queue_.Dequeue(timeResolution_));

      if (completion.get() != NULL)
      {
        dynamic_cast<Notification&>(*completion).NotifyClient();
      }
    }
  };


  NativeEnvironment::Lock::Lock(NativeEnvironment& that) :
    that_(that),
    lock_(that.mutex_)
  {
    firstLock_ = (that.countLocks_ == 0);
    that.countLocks_++;
  }


  NativeEnvironment::Lock::~Lock()
  {
    assert(that_.countLocks_ > 0);
    that_.countLocks_--;
  }


  NativeEnvironment::NativeEnvironment(unsigned int timeResolution) :
    notificationThread_(new NotificationRunnable(notificationQueue_, timeResolution), 0 /* time resolution is in Dequeue() */),
    countLocks_(0)
  {
  }


  void NativeEnvironment::NotifyOracleSuccess(const boost::weak_ptr<IOracleClient>& client,
                                              IOracleCommand* command /* takes ownership */,
                                              IMessage* result /* takes ownership */)
  {
    notificationQueue_.Enqueue(new SuccessNotification(*this, client, command, result));
  }


  void NativeEnvironment::NotifyOracleError(const boost::weak_ptr<IOracleClient>& client,
                                            IOracleCommand* command /* takes ownership */,
                                            const Orthanc::OrthancException& error)
  {
    notificationQueue_.Enqueue(new ErrorNotification(*this, client, command, error));
  }
}
