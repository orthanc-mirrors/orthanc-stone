/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "StartupParametersBuilder.h"
#include <iostream>
#include <cstdio>
#include "emscripten/html5.h"

namespace OrthancStone
{
  void StartupParametersBuilder::Clear()
  {
    startupParameters_.clear();
  }

  void StartupParametersBuilder::SetStartupParameter(
    const char* name,
    const char* value)
  {
    startupParameters_.push_back(std::make_tuple(name, value));
  }

  void StartupParametersBuilder::GetStartupParameters(
    boost::program_options::variables_map& parameters,
    const boost::program_options::options_description& options) 
  {
    std::vector<std::string> argvStrings(startupParameters_.size() + 1);
    // argv mirrors pointers to the internal argvStrings buffers.
    // ******************************************************
    // THIS IS HIGHLY DANGEROUS SO BEWARE!!!!!!!!!!!!!!
    // ******************************************************
    std::vector<const char*> argv(startupParameters_.size() + 1);
    
    int argCounter = 0;
    argvStrings[argCounter] = "dummy.exe";
    argv[argCounter] = argvStrings[argCounter].c_str();
    
    argCounter++;
    
    std::string cmdLine = "";
    for ( StartupParameters::const_iterator it = startupParameters_.begin(); 
          it != startupParameters_.end(); 
          it++)
    {
      std::stringstream argSs;

      argSs << "--" << std::get<0>(*it);
      if(std::get<1>(*it).length() > 0)
        argSs << "=" << std::get<1>(*it);
      
      argvStrings[argCounter] = argSs.str();
      cmdLine = cmdLine + " " + argvStrings[argCounter];
      std::cout << cmdLine << std::endl;
      argv[argCounter] = argvStrings[argCounter].c_str();
      argCounter++;
    }


    std::cout << "simulated cmdLine = \"" << cmdLine.c_str() << "\"\n";

    try
    {
      boost::program_options::store(
        boost::program_options::command_line_parser(argCounter, argv.data()).
          options(options).allow_unregistered().run(), parameters);
      boost::program_options::notify(parameters);
    }
    catch (boost::program_options::error& e)
    {
      std::cerr << "Error while parsing the command-line arguments: " <<
        e.what() << std::endl;    
    }
  }
}
