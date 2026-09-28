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


#pragma once

#include "../../Oracle/IEnvironment.h"
#include "RunnableThread.h"

#include <MultiThreading/SharedMessageQueue.h>


namespace OrthancStone
{
  class NativeEnvironment : public IEnvironment
  {
  private:
    class NotificationRunnable;
    class Notification;
    class SuccessNotification;
    class ErrorNotification;

    // TODO Refactoring - Is it necessary to have "recursive_mutex", not plain "mutex"?
    boost::recursive_mutex       mutex_;  // Main mutex of the application, to go single-threaded
    Orthanc::SharedMessageQueue  notificationQueue_;
    RunnableThread               notificationThread_;
    unsigned int                 countLocks_;  // Only for debugging

  public:
    class Lock : public ILock
    {
    private:
      NativeEnvironment&                   that_;
      boost::recursive_mutex::scoped_lock  lock_;
      bool                                 firstLock_;

    public:
      Lock(NativeEnvironment& that);

      virtual ~Lock() ORTHANC_OVERRIDE;

      bool IsFirstLock() const  // For debugging
      {
        return firstLock_;
      }
    };


    NativeEnvironment(unsigned int timeResolution /* in milliseconds */);

    void Start()
    {
      notificationThread_.Start();
    }

    void Stop()
    {
      notificationThread_.Stop();
    }

    virtual void NotifyOracleSuccess(const boost::weak_ptr<IOracleClient>& client,
                                     IOracleCommand* command /* takes ownership */,
                                     IMessage* result /* takes ownership */) ORTHANC_OVERRIDE;

    virtual void NotifyOracleError(const boost::weak_ptr<IOracleClient>& client,
                                   IOracleCommand* command /* takes ownership */,
                                   const Orthanc::OrthancException& error) ORTHANC_OVERRIDE;

    virtual ILock* AcquireLock() ORTHANC_OVERRIDE
    {
      return new Lock(*this);
    }
  };
}
