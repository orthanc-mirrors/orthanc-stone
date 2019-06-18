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

#pragma once

#include "IMessageEmitter.h"
#include "IObservable.h"

#include <boost/thread.hpp>

namespace OrthancStone
{
  /**
   * This class is used when using the ThreadedOracle : since messages
   * can be sent from multiple Oracle threads, this IMessageEmitter
   * implementation serializes the callbacks.
   * 
   * The internal mutex used in Oracle messaging can also be used to 
   * protect the application data. Thus, this class can be used as a single
   * application-wide mutex.
   */
  class LockingEmitter : public IMessageEmitter
  {
  private:
    boost::shared_mutex                mutex_;
    MessageBroker        broker_;
    IObservable          oracleObservable_;

  public:
    LockingEmitter() :
      oracleObservable_(broker_)
    {
    }

    MessageBroker& GetBroker()
    {
      return broker_;
    }

    virtual void EmitMessage(const IObserver& observer,
      const IMessage& message) ORTHANC_OVERRIDE
    {
      try
      {
        boost::unique_lock<boost::shared_mutex>  lock(mutex_);
        oracleObservable_.EmitMessage(observer, message);
      }
      catch (Orthanc::OrthancException& e)
      {
        LOG(ERROR) << "Exception while emitting a message: " << e.What();
      }
    }


    class ReaderLock : public boost::noncopyable
    {
    private:
      LockingEmitter& that_;
      boost::shared_lock<boost::shared_mutex>  lock_;

    public:
      ReaderLock(LockingEmitter& that) :
        that_(that),
        lock_(that.mutex_)
      {
      }
    };


    class WriterLock : public boost::noncopyable
    {
    private:
      LockingEmitter& that_;
      boost::unique_lock<boost::shared_mutex>  lock_;

    public:
      WriterLock(LockingEmitter& that) :
        that_(that),
        lock_(that.mutex_)
      {
      }

      MessageBroker& GetBroker()
      {
        return that_.broker_;
      }

      IObservable& GetOracleObservable()
      {
        return that_.oracleObservable_;
      }
    };
  };
}
