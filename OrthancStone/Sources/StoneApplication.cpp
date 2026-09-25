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
  StoneApplication::Configuration::Configuration() :
    isLocalOrthanc_(false),
    rootDirectory_("."),
    oracleThreadsCount_(4),
    workersTimeResolution_(50),  // By default, time resolution of 50ms
    dicomCacheSize_(0)  // By default, no DICOM cache
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
  class StoneApplication::PImpl
  {
  private:
    WebAssemblyEnvironment  environment_;
    WebAssemblyOracle       oracle_;

  public:
    PImpl(const Configuration& configuration) :
      oracle_(configuration)
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
    PImpl(const Configuration& configuration) :
      oracle_(configuration)
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

  StoneApplication::StoneApplication(const Configuration& configuration)
  {
#if ORTHANC_STONE_TARGET_PLATFORM_WASM == 1
    pimpl_ = new PImpl(configuration);
#elif ORTHANC_STONE_TARGET_PLATFORM_NATIVE == 1
    pimpl_ = new PImpl(configuration);
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


  void StoneApplication::Initialize(const Configuration& configuration)
  {
    Orthanc::Mutex::ScopedLock lock(applicationMutex_);

    if (application_.get() == NULL)
    {
      application_.reset(new StoneApplication(configuration));
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
}
