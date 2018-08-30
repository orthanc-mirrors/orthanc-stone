/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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

#include "../IStoneApplication.h"

#if ORTHANC_ENABLE_NATIVE != 1
#error this file shall be included only with the ORTHANC_ENABLE_NATIVE set to 1
#endif

namespace OrthancStone
{
  class NativeStoneApplicationContext;

  class NativeStoneApplicationRunner
  {
  protected:
    MessageBroker&      broker_;
    IStoneApplication&  application_;
  public:

    NativeStoneApplicationRunner(MessageBroker& broker,
                                 IStoneApplication& application)
      : broker_(broker),
        application_(application)
    {
    }
    int Execute(int argc,
                char* argv[]);

    virtual void Initialize() = 0;
    virtual void DeclareCommandLineOptions(boost::program_options::options_description& options) = 0;
    virtual void ParseCommandLineOptions(const boost::program_options::variables_map& parameters) = 0;

    virtual void Run(NativeStoneApplicationContext& context, const std::string& title, int argc, char* argv[]) = 0;
    virtual void Finalize() = 0;
  };

}
