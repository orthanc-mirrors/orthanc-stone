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

#include <OrthancFramework.h>  // To have the macros properly defined
#include <Compatibility.h>  // ORTHANC_OVERRIDE

#if !defined(ORTHANC_ENABLE_THREADS)
#  error The macro ORTHANC_ENABLE_THREADS must be defined
#endif

#if !defined(ORTHANC_ENABLE_DCMTK)
#  error The macro ORTHANC_ENABLE_DCMTK must be defined
#endif

#if ORTHANC_ENABLE_THREADS != 1
#  error This file can only compiled for native targets
#endif

#if ORTHANC_ENABLE_DCMTK == 1
#  include "../../Toolbox/ParsedDicomCache.h"
#endif

#include "../../Messages/IMessageEmitter.h"
#include "../../Oracle/IOracle.h"
#include "../../Oracle/OracleCallback.h"
#include "../../StoneApplication.h"
#include "RunnableThread.h"

#include <MultiThreading/ThreadPool.h>


namespace OrthancStone
{
  class ThreadedOracle : public IOracle
  {
  private:
    class GenericRunnable;
    class SleepRunnable;

    StoneApplication::Configuration  configuration_;
    RunnableThread                   sleepingThread_;
    Orthanc::ThreadPool              threadPool_;

#if ORTHANC_ENABLE_DCMTK == 1
    boost::shared_ptr<ParsedDicomCache>  dicomCache_;
#endif

    void SubmitInternal(IOracleCallback* callback /* takes ownership */);

  public:
    ThreadedOracle(const StoneApplication::Configuration& configuration);

    void Start();

    void Stop();

    virtual void Submit(IEnvironment& environment,
                        const boost::shared_ptr<IOracleClient>& client,
                        IOracleCommand* command /* takes ownership */) ORTHANC_OVERRIDE;
  };
}
