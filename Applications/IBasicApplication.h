/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "BasicApplicationContext.h"

#include <boost/program_options.hpp>

#if ORTHANC_ENABLE_SDL == 1
#  include <SDL.h>   // Necessary to avoid undefined reference to `SDL_main'
#endif

namespace OrthancStone
{
  class IBasicApplication : public boost::noncopyable
  {
  public:
    virtual ~IBasicApplication()
    {
    }

    virtual void DeclareCommandLineOptions(boost::program_options::options_description& options) = 0;

    virtual std::string GetTitle() const = 0;

    virtual void Initialize(BasicApplicationContext& context,
                            IStatusBar& statusBar,
                            const boost::program_options::variables_map& parameters) = 0;

    virtual void Finalize() = 0;

#if ORTHANC_ENABLE_SDL == 1
    static int ExecuteWithSdl(IBasicApplication& application,
                              int argc, 
                              char* argv[]);
#endif
  };
}
