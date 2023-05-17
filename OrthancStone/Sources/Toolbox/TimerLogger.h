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


#pragma once

#include <boost/noncopyable.hpp>
#include <string>

#if defined(__EMSCRIPTEN__)
#  include <emscripten.h>
#else
#  include <boost/date_time/posix_time/posix_time_types.hpp>
#endif


namespace OrthancStone
{
  class TimerLogger : public boost::noncopyable
  {
  private:
    std::string name_;

#if defined(__EMSCRIPTEN__)
    double start_;
#else
    boost::posix_time::ptime start_;
#endif

  public:
    TimerLogger(const std::string& name);

    ~TimerLogger();
  };
}
