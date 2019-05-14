#include "StartupParametersBuilder.h"

namespace OrthancStone
{
    void StartupParametersBuilder::Clear() {
        startupParameters_.clear();
    }

    void StartupParametersBuilder::SetStartupParameter(const char* name, const char* value) {
        startupParameters_.push_back(std::make_tuple(name, value));
    }

    void StartupParametersBuilder::GetStartupParameters(boost::program_options::variables_map& parameters, const boost::program_options::options_description& options) {
        
        const char* argv[startupParameters_.size() + 1];
        int argCounter = 0;
        argv[0] = "Toto.exe";
        argCounter++;

        std::string cmdLine = "";
        for (StartupParameters::const_iterator it = startupParameters_.begin(); it != startupParameters_.end(); it++) {
            char* arg = new char[128];
            snprintf(arg, 128, "--%s=%s", std::get<0>(*it).c_str(), std::get<1>(*it).c_str());
            argv[argCounter] = arg;
            cmdLine = cmdLine + " --" + std::get<0>(*it) + "=" + std::get<1>(*it);
            argCounter++;
        }

        printf("simulated cmdLine = %s\n", cmdLine.c_str());

        try
        {
            boost::program_options::store(boost::program_options::command_line_parser(argCounter, argv).
                                            options(options).allow_unregistered().run(), parameters);
            boost::program_options::notify(parameters);
        }
        catch (boost::program_options::error& e)
        {
            printf("Error while parsing the command-line arguments: %s\n", e.what());
        }

    }
}