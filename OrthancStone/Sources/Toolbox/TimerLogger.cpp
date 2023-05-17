/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include "TimerLogger.h"

#include <Logging.h>


namespace OrthancStone
{
  TimerLogger::TimerLogger(const std::string& name) :
    name_(name)
  {
#if defined(__EMSCRIPTEN__)
    start_ = emscripten_get_now();
#else
    start_ = boost::posix_time::microsec_clock::universal_time();
#endif
  }

  
  TimerLogger::~TimerLogger()
  {
#if defined(__EMSCRIPTEN__)
    int elapsed = static_cast<int>(round(emscripten_get_now() - start_));
#else
    const boost::posix_time::ptime end = boost::posix_time::microsec_clock::universal_time();
    int elapsed = (end - start_).total_milliseconds();
#endif

    LOG(WARNING) << name_ << " - Elapsed time: " << elapsed << "ms";
  }
}
