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


#include "StoneApplication.h"

#include "StoneException.h"

#include <Compatibility.h>
#include <Logging.h>
#include <MultiThreading/Mutex.h>


#if ORTHANC_STONE_TARGET_PLATFORM_WASM == 1
#  include "Platforms/WebAssembly/WebAssemblyEnvironment.h"
#  include "Platforms/WebAssembly/WebAssemblyOracle.h"
#elif ORTHANC_STONE_TARGET_PLATFORM_NATIVE == 1
#  include "Platforms/Native/NativeEnvironment.h"
#  include "Platforms/Native/ThreadedOracle.h"
#else
#  error Support your platform here
#endif

namespace OrthancStone
{
  StoneApplication::Configuration::Configuration() :
    isLocalOrthanc_(false),
    rootDirectory_("."),
    oracleThreadsCount_(4),
    workersTimeResolution_(50),  // By default, time resolution of 50ms
    dicomCacheSize_(0)  // By default, the cache is disabled
  {
  }


  void StoneApplication::Configuration::SetRemoteOrthancParameters(const Orthanc::WebServiceParameters& orthanc)
  {
    isLocalOrthanc_ = false;
    remoteOrthanc_ = orthanc;
  }


  const Orthanc::WebServiceParameters& StoneApplication::Configuration::GetRemoteOrthancParameters() const
  {
    if (isLocalOrthanc_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return remoteOrthanc_;
    }
  }


  void StoneApplication::Configuration::SetLocalOrthancRoot(const std::string& root)
  {
    isLocalOrthanc_ = true;
    localOrthancRoot_ = root;
  }


  const std::string& StoneApplication::Configuration::GetLocalOrthancRoot() const
  {
    if (isLocalOrthanc_)
    {
      return localOrthancRoot_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  void StoneApplication::Configuration::SetOracleThreadsCount(unsigned int count)
  {
    if (count == 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      oracleThreadsCount_ = count;
    }
  }


  void StoneApplication::Configuration::SetWorkersTimeResolution(unsigned int milliseconds)
  {
    if (milliseconds == 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      workersTimeResolution_ = milliseconds;
    }
  }


#if ORTHANC_STONE_TARGET_PLATFORM_WASM == 1
  class StoneApplication::Context::PImpl
  {
  private:
    class Emitter : public IMessageEmitter  // TODO Refactoring - Remove this
    {
    private:
      IObservable  oracleObservable_;

    public:
      void EmitMessage(boost::weak_ptr<IObserver> observer,
                       const IMessage& message) ORTHANC_OVERRIDE
      {
        oracleObservable_.EmitMessage(observer, message);
      }

      IObservable& GetOracleObservable()
      {
        return oracleObservable_;
      }
    };

    WebAssemblyEnvironment  environment_;
    WebAssemblyOracle       oracle_;
    Emitter                 emitter_;

  public:
    PImpl(const Configuration& configuration) :
      oracle_(configuration)
    {
    }

    IEnvironment& GetEnvironment()
    {
      return environment_;
    }

    WebAssemblyOracle& GetOracle()
    {
      return oracle_;
    }

    IMessageEmitter& GetMessageEmitter()
    {
      return emitter_;
    }

    IObservable& GetOracleObservable()
    {
      return emitter_.GetOracleObservable();
    }

    void Start()
    {
    }

    void Stop()
    {
    }
  };
#endif


#if ORTHANC_STONE_TARGET_PLATFORM_NATIVE == 1
  class StoneApplication::Context::PImpl
  {
  private:
    class Emitter : public IMessageEmitter  // TODO Refactoring - Remove this
    {
    private:
      NativeEnvironment&  environment_;
      IObservable         oracleObservable_;

    public:
      Emitter(NativeEnvironment& environment) :
        environment_(environment)
      {
      }

      void EmitMessage(boost::weak_ptr<IObserver> observer,
                       const IMessage& message) ORTHANC_OVERRIDE
      {
        NativeEnvironment::Lock lock(environment_);

        if (lock.IsFirstLock())  // For debugging
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }

        oracleObservable_.EmitMessage(observer, message);
      }

      IObservable& GetOracleObservable()
      {
        return oracleObservable_;
      }
    };


    NativeEnvironment  environment_;
    ThreadedOracle     oracle_;
    Emitter            emitter_;

  public:
    PImpl(const Configuration& configuration) :
      environment_(configuration.GetWorkersTimeResolution()),
      oracle_(configuration),
      emitter_(environment_)
    {
    }

    IEnvironment& GetEnvironment()
    {
      return environment_;
    }

    ThreadedOracle& GetOracle()
    {
      return oracle_;
    }

    IMessageEmitter& GetMessageEmitter()
    {
      return emitter_;
    }

    IObservable& GetOracleObservable()
    {
      return emitter_.GetOracleObservable();
    }

    void Start()
    {
      environment_.Start();
      oracle_.Start();
    }

    void Stop()
    {
      oracle_.Stop();
      environment_.Stop();
    }
  };
#endif


  StoneApplication::Context::Context(const Configuration& configuration)
  {
#if ORTHANC_STONE_TARGET_PLATFORM_WASM == 1
    pimpl_ = new PImpl(configuration);
#elif ORTHANC_STONE_TARGET_PLATFORM_NATIVE == 1
    pimpl_ = new PImpl(configuration);
#else
#   error Support your platform here
#endif
  }


  StoneApplication::Context::~Context()
  {
    assert(pimpl_ != NULL);
    delete pimpl_;
  }


  IEnvironment& StoneApplication::Context::GetEnvironment()
  {
    assert(pimpl_ != NULL);
    return pimpl_->GetEnvironment();
  }


  IOracle& StoneApplication::Context::GetOracle()
  {
    assert(pimpl_ != NULL);
    return pimpl_->GetOracle();
  }


  void StoneApplication::Context::EmitMessage(boost::weak_ptr<IObserver> observer,
                                              const IMessage& message)
  {
    assert(pimpl_ != NULL);
    return pimpl_->GetMessageEmitter().EmitMessage(observer, message);
  }


  IObservable& StoneApplication::Context::GetOracleObservable()
  {
    assert(pimpl_ != NULL);
    return pimpl_->GetOracleObservable();
  }


  bool StoneApplication::Run()
  {
    try
    {
      boost::shared_ptr<Context> context(new Context(configuration_));

      assert(context.get() != NULL);
      assert(context->pimpl_ != NULL);

      context->pimpl_->Start();

      RunInternal(context);

      context->pimpl_->Stop();

      return true;
    }
    catch (Orthanc::OrthancException& e)
    {
      LOG(ERROR) << "OrthancException: " << e.What();
      return false;
    }
    catch (StoneException& e)
    {
      LOG(ERROR) << "StoneException: " << e.What();
      return false;
    }
    catch (std::runtime_error& e)
    {
      LOG(ERROR) << "Runtime error: " << e.what();
      return false;
    }
    catch (...)
    {
      LOG(ERROR) << "Native exception";
      return false;
    }
  }


  static Orthanc::Mutex                              applicationMutex_;  // TODO Refactoring - Remove this
  static std::unique_ptr<StoneApplication::Context>  application_;  // TODO Refactoring - Remove this

  StoneApplication::Context& StoneApplication::GetInstance()
  {
    Orthanc::Mutex::ScopedLock lock(applicationMutex_);

    if (application_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return *application_;
  }


  void StoneApplication::Initialize(const Configuration& configuration)
  {
    Orthanc::Mutex::ScopedLock lock(applicationMutex_);

    if (application_.get() == NULL)
    {
      application_.reset(new StoneApplication::Context(configuration));
      application_->pimpl_->Start();
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  void StoneApplication::Finalize()
  {
    Orthanc::Mutex::ScopedLock lock(applicationMutex_);

    if (application_.get() != NULL)
    {
      application_->pimpl_->Stop();
      application_.reset(NULL);
    }
  }
}
