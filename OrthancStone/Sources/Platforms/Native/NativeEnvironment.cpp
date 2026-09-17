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


namespace OrthancStone
{
  class NativeEnvironment::Completion : public Orthanc::IDynamicObject
  {
  protected:
    boost::weak_ptr<IOracleClient>   client_;
    std::unique_ptr<IOracleCommand>  command_;

  public:
    Completion(const boost::weak_ptr<IOracleClient>& client,
               IOracleCommand* command /* takes ownership */) :
      client_(client),
      command_(command)
    {
      if (command == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    virtual void NotifyClient() = 0;
  };


  class NativeEnvironment::SuccessCompletion : public Completion
  {
  private:
    std::unique_ptr<Orthanc::IDynamicObject> result_;

  public:
    SuccessCompletion(const boost::weak_ptr<IOracleClient>& client,
                      IOracleCommand* command /* takes ownership */,
                      Orthanc::IDynamicObject* result /* takes ownership */) :
      Completion(client, command),
      result_(result)
    {
      if (result == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }

    virtual void NotifyClient() ORTHANC_OVERRIDE
    {
      boost::shared_ptr<IOracleClient> lock(client_.lock());

      if (lock)
      {
        lock->HandleSuccessFromOracle(*command_, *result_);
      }
    }
  };


  class NativeEnvironment::ErrorCompletion : public Completion
  {
  private:
    Orthanc::OrthancException  error_;

  public:
    ErrorCompletion(const boost::weak_ptr<IOracleClient>& client,
                    IOracleCommand* command /* takes ownership */,
                    const Orthanc::OrthancException& error) :
      Completion(client, command),
      error_(error)
    {
    }

    virtual void NotifyClient() ORTHANC_OVERRIDE
    {
      boost::shared_ptr<IOracleClient> lock(client_.lock());

      if (lock)
      {
        lock->HandleErrorFromOracle(*command_, error_);
      }
    }
  };


  class NativeEnvironment::OracleRunnable : public Orthanc::IRunnable
  {
  private:
    Orthanc::SharedMessageQueue& queue_;

  public:
    OracleRunnable(Orthanc::SharedMessageQueue& queue) :
      queue_(queue)
    {
    }

    virtual void Run() ORTHANC_OVERRIDE
    {
      std::unique_ptr<Orthanc::IDynamicObject> completion(queue_.Dequeue(50 /* milliseconds */));

      if (completion.get() != NULL)
      {
        dynamic_cast<Completion&>(*completion).NotifyClient();
      }
    }
  };


  NativeEnvironment::NativeEnvironment() :
    oracleThread_(new OracleRunnable(oracleQueue_), 0 /* time resolution is in Dequeue() */)
  {
  }


  void NativeEnvironment::NotifyOracleSuccess(const boost::weak_ptr<IOracleClient>& client,
                                              IOracleCommand* command /* takes ownership */,
                                              Orthanc::IDynamicObject* result /* takes ownership */)
  {
    oracleQueue_.Enqueue(new SuccessCompletion(client, command, result));
  }


  void NativeEnvironment::NotifyOracleError(const boost::weak_ptr<IOracleClient>& client,
                                            IOracleCommand* command /* takes ownership */,
                                            const Orthanc::OrthancException& error)
  {
    oracleQueue_.Enqueue(new ErrorCompletion(client, command, error));
  }
}
