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

#include <Compatibility.h>
#include <Logging.h>
#include <MultiThreading/Mutex.h>


#if ORTHANC_STONE_TARGET_PLATFORM_WASM == 1
#  include "Platforms/WebAssembly/WebAssemblyEnvironment.h"
#  include "Platforms/WebAssembly/WebAssemblyOracle.h"
#elif ORTHANC_STONE_TARGET_PLATFORM_NATIVE == 1
#  include "Platforms/Native/NativeEnvironment.h"
#  include "Oracle/ThreadedOracle.h"
#else
#  error Support your platform here
#endif

namespace OrthancStone
{
#if ORTHANC_STONE_TARGET_PLATFORM_WASM == 1
  class StoneApplication::PImpl
  {
  private:
    WebAssemblyEnvironment  environment_;
    New::WebAssemblyOracle  oracle_;

  public:
    IEnvironment& GetEnvironment()
    {
      return environment_;
    }

    New::IOracle& GetOracle()
    {
      return oracle_;
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
  class StoneApplication::PImpl
  {
  private:
    NativeEnvironment    environment_;
    New::ThreadedOracle  oracle_;

  public:
    PImpl(unsigned int oracleThreads) :
      oracle_(oracleThreads)
    {
    }

    IEnvironment& GetEnvironment()
    {
      return environment_;
    }

    New::IOracle& GetOracle()
    {
      return oracle_;
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


  static Orthanc::Mutex                     applicationMutex_;
  static std::unique_ptr<StoneApplication>  application_;
  static unsigned int                       threadsCount_ = 4;

  StoneApplication::StoneApplication()
  {
#if ORTHANC_STONE_TARGET_PLATFORM_WASM == 1
    pimpl_ = new PImpl;
#elif ORTHANC_STONE_TARGET_PLATFORM_NATIVE == 1
    pimpl_ = new PImpl(threadsCount_);
#else
#   error Support your platform here
#endif
  }


  StoneApplication::~StoneApplication()
  {
    assert(pimpl_ != NULL);
    delete pimpl_;
  }


  StoneApplication& StoneApplication::GetInstance()
  {
    Orthanc::Mutex::ScopedLock lock(applicationMutex_);

    if (application_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    return *application_;
  }


  void StoneApplication::Initialize()
  {
    Orthanc::Mutex::ScopedLock lock(applicationMutex_);

    if (application_.get() == NULL)
    {
      application_.reset(new StoneApplication);
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


  IEnvironment& StoneApplication::GetEnvironment()
  {
    assert(pimpl_ != NULL);
    return pimpl_->GetEnvironment();
  }


  void StoneApplication::Submit(const boost::shared_ptr<IOracleClient>& client,
                                IOracleCommand* command /* takes ownership */)
  {
    assert(pimpl_ != NULL);
    pimpl_->GetOracle().Submit(pimpl_->GetEnvironment(), client, command);
  }


  void StoneApplication::SetThreadsCount(unsigned int count)
  {
    if (count == 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      Orthanc::Mutex::ScopedLock lock(applicationMutex_);
      threadsCount_ = count;
    }
  }
}
