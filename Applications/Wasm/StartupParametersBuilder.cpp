#include "StartupParametersBuilder.h"
#include <iostream>

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
    argvStrings[argCounter] = "Toto.exe";
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
        argv[argCounter] = argvStrings[argCounter].c_str();
        argCounter++;
    }

    std::cout << "simulated cmdLine = \"" << cmdLine.c_str() << "\"\n";

    try
    {
      boost::program_options::store(
        boost::program_options::command_line_parser(argCounter, argv.data()).
          options(options).run(), parameters);
      boost::program_options::notify(parameters);
    }
    catch (boost::program_options::error& e)
    {
      std::cerr << "Error while parsing the command-line arguments: " <<
        e.what() << std::endl;    }
  }
}
