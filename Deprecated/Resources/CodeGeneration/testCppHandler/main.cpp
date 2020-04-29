#include <string>
#include <fstream>
#include <filesystem>
#include <regex>
using namespace std;
namespace fs = std::filesystem;

#include <boost/program_options.hpp>
using namespace boost::program_options;

#include "TestStoneCodeGen_generated.hpp"

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

string SlurpFile(const string& fileName)
{
  ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

  ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, ios::beg);

  vector<char> bytes(fileSize);
  ifs.read(bytes.data(), fileSize);

  return string(bytes.data(), fileSize);
}

class MyHandler : public TestStoneCodeGen::IHandler
{
public:
  virtual bool Handle(const TestStoneCodeGen::A& value) override
  {
    TestStoneCodeGen::StoneDumpValue(cout, value);
    return true;
  }
  virtual bool Handle(const TestStoneCodeGen::B& value) override
  {
    TestStoneCodeGen::StoneDumpValue(cout, value);
    return true;
  }
  virtual bool Handle(const TestStoneCodeGen::C& value) override
  {
    TestStoneCodeGen::StoneDumpValue(cout, value);
    return true;
  }
  virtual bool Handle(const TestStoneCodeGen::Message1& value) override
  {
    TestStoneCodeGen::StoneDumpValue(cout, value);
    return true;
  }
  virtual bool Handle(const TestStoneCodeGen::Message2& value) override
  {
    TestStoneCodeGen::StoneDumpValue(cout, value);
    return true;
  }
};

template<typename T>
void ProcessPath(T filePath)
{
  cout << "+--------------------------------------------+\n";
  cout << "| Processing: " << filePath.path().string() << "\n";
  cout << "+--------------------------------------------+\n";
  MyHandler handler;
  auto contents = SlurpFile(filePath.path().string());
  TestStoneCodeGen::StoneDispatchToHandler(contents, &handler);
}

int main(int argc, char** argv)
{
  try
  {

    options_description desc("Allowed options");
    desc.add_options()
      // First parameter describes option name/short name
      // The second is parameter to option
      // The third is description
      ("help,h", "print usage message")
      ("pattern,p", value<string>(), "pattern for input")
      ;

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    if (vm.count("help"))
    {
      cout << desc << "\n";
      return 0;
    }

    notify(vm);

    string pattern = vm["pattern"].as<string>();

    // tranform globbing pattern into regex
    // we should deal with -, ., *...
    string regexPatternStr = pattern;
    cout << "Pattern is: " << regexPatternStr << endl;
    ReplaceInString(regexPatternStr, "\\", "\\\\");
    ReplaceInString(regexPatternStr, "-", "\\-");
    ReplaceInString(regexPatternStr, ".", "\\.");
    ReplaceInString(regexPatternStr, "*", ".*");
    ReplaceInString(regexPatternStr, "?", ".");
    cout << "Corresponding regex is: " << regexPatternStr << endl;

    regex regexPattern(regexPatternStr);

    for (auto& p : fs::directory_iterator("."))
    {
      auto fileName = p.path().filename().string();
      if (regex_match(fileName, regexPattern))
      {
        ProcessPath(p);
      }
    }
    return 0;


  }
  catch (exception& e)
  {
    cerr << e.what() << "\n";
  }
}