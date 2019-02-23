#include <string>
#include <filesystem>
#include <regex>
using namespace std;
namespace fs = std::filesystem;

#include <boost/program_options.hpp>
using namespace boost::program_options;

#include "VsolMessages_generated.hpp"

/**
Transforms `str` by replacing occurrences of `oldStr` with `newStr`, using 
plain text (*not* regular expressions.)
*/
static inline void ReplaceInString(
  string& str,
  const std::string& oldStr,
  const std::string& newStr)
{
  std::string::size_type pos = 0u;
  while ((pos = str.find(oldStr, pos)) != std::string::npos) {
    str.replace(pos, oldStr.length(), newStr);
    pos += newStr.length();
  }
}

int main(int argc, char** argv)
{
  try
  {
    string pattern;

    options_description desc("Allowed options");
    desc.add_options()
      // First parameter describes option name/short name
      // The second is parameter to option
      // The third is description
      ("help,h", "print usage message")
      ("pattern,p", value(&pattern), "pattern for input")
      ;

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    if (vm.count("help"))
    {
      cout << desc << "\n";
      return 0;
    }

    // tranform globbing pattern into regex
    // we should deal with -, ., *...
    string regexPatternStr = pattern;
    regex regexPattern(regexPatternStr);

    for (auto& p : fs::directory_iterator("."))
    {
      if (regex_match(p.path().string(), regexPattern))
        std::cout << "\"" << p << "\" is a match\n";
      else
        std::cout << "\"" << p << "\" is *not* a match\n";
    }
    return 0;


  }
  catch (exception& e)
  {
    cerr << e.what() << "\n";
  }
}