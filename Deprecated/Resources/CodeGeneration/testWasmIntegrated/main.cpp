#include <iostream>
#include <sstream>
#include <emscripten/emscripten.h>
#include "TestStoneCodeGen_generated.hpp"

using std::stringstream;

int main()
{
    std::cout << "Hello world from testWasmIntegrated! (this is sent from C++)" << std::endl;
    try
    {
    const char* jsonData = R"bgo({"definition":
    {
      "val" : [ "berk", 42 ],
      "zozo" : { "23": "zloutch", "lalala": 42}
    }
    })bgo";
    std::string strValue(jsonData);
    
    Json::Value readValue;

    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();

    StoneSmartPtr<Json::CharReader> ptr(reader);

    std::string errors;

    bool ok = reader->parse(
      strValue.c_str(),
      strValue.c_str() + strValue.size(),
      &readValue,
      &errors
    );
    if (!ok)
    {
      std::stringstream ss;
      ss << "Jsoncpp parsing error: " << errors;
      throw std::runtime_error(ss.str());
    }
    std::cout << "Json parsing OK" << std::endl;
    std::cout << readValue  << std::endl;
    }
    catch(std::exception& e)
    {
      std::cout << "Json parsing THROW" << std::endl;
      std::cout << "e.what() = " << e.what() << std::endl;
    }
}

extern "C" void SendMessageFromCppJS(const char* message);
extern "C" void SendFreeTextFromCppJS(const char* message);

#define HANDLE_MESSAGE(Type,value) \
  stringstream ss; \
  ss << "Received an instance of:\n" #Type "\n. Here's the dump:\n"; \
  TestStoneCodeGen::StoneDumpValue(ss, value, 0); \
  SendFreeTextFromCppJS(ss.str().c_str()); \
  return true;

#define ECHO_MESSAGE(Type,value) \
  stringstream ss; \
  ss << "Received an instance of:\n" #Type "\n. Here's the dump:\n"; \
  TestStoneCodeGen::StoneDumpValue(ss, value, 0); \
  SendFreeTextFromCppJS(ss.str().c_str()); \
  std::string serializedInCpp = StoneSerialize(value); \
  SendMessageFromCppJS(serializedInCpp.c_str()); \
  return true;

class MyHandler : public TestStoneCodeGen::IHandler
{
  public:
    virtual bool Handle(const TestStoneCodeGen::A& value) override
    {
      HANDLE_MESSAGE(TestStoneCodeGen::A,value)
    }
    virtual bool Handle(const TestStoneCodeGen::B& value) override
    {
      HANDLE_MESSAGE(TestStoneCodeGen::B,value)
    }

    virtual bool Handle(const TestStoneCodeGen::Message1& value) override
    {
      HANDLE_MESSAGE(TestStoneCodeGen::Message1,value)
    }

    virtual bool Handle(const TestStoneCodeGen::Message2& value) override
    {
      HANDLE_MESSAGE(TestStoneCodeGen::Message2,value)
    }

    virtual bool Handle(const TestStoneCodeGen::C& value) override
    {
      HANDLE_MESSAGE(TestStoneCodeGen::C,value)
    }
};

class MyEchoHandler : public TestStoneCodeGen::IHandler
{
  public:
    virtual bool Handle(const TestStoneCodeGen::A& value) override
    {
      ECHO_MESSAGE(TestStoneCodeGen::A,value)
    }
    virtual bool Handle(const TestStoneCodeGen::B& value) override
    {
      ECHO_MESSAGE(TestStoneCodeGen::B,value)
    }

    virtual bool Handle(const TestStoneCodeGen::Message1& value) override
    {
      ECHO_MESSAGE(TestStoneCodeGen::Message1,value)
    }

    virtual bool Handle(const TestStoneCodeGen::Message2& value) override
    {
      ECHO_MESSAGE(TestStoneCodeGen::Message2,value)
    }

    virtual bool Handle(const TestStoneCodeGen::C& value) override
    {
      ECHO_MESSAGE(TestStoneCodeGen::C,value)
    }
};

extern "C" void EMSCRIPTEN_KEEPALIVE SendMessageToCpp(const char* message)
{
    MyHandler handler;
    try
    {
      bool handled = TestStoneCodeGen::StoneDispatchToHandler(message,&handler);
      if(!handled)
      {
        SendFreeTextFromCppJS("This message is valid JSON, but was not handled!");  
      }
    }
    catch(std::exception& e)
    {
      stringstream ss;
      ss << "Error while parsing message: " << e.what() << "\n";
      SendFreeTextFromCppJS(ss.str().c_str());  
    }
}

extern "C" void EMSCRIPTEN_KEEPALIVE SendMessageToCppForEcho(const char* message)
{
    MyEchoHandler echoHandler;
    try
    {
      bool handled = TestStoneCodeGen::StoneDispatchToHandler(message,&echoHandler);
      if(!handled)
      {
        SendFreeTextFromCppJS("This message is valid JSON, but was not handled by the echo handler!");  
      }
    }
    catch(std::exception& e)
    {
      stringstream ss;
      ss << "Error while parsing message: " << e.what() << "\n";
      SendFreeTextFromCppJS(ss.str().c_str());  
    }
}

void EMSCRIPTEN_KEEPALIVE StartWasmApplication(const char* baseUri)
{
    printf("Hello! (this is sent from C++)\n");

//     // recreate a command line from uri arguments and parse it
//     boost::program_options::variables_map parameters;
//     boost::program_options::options_description options;
//     application->DeclareStartupOptions(options);
//     startupParametersBuilder.GetStartupParameters(parameters, options);

//     context.reset(new OrthancStone::StoneApplicationContext(broker));
//     context->SetOrthancBaseUrl(baseUri);
//     printf("Base URL to Orthanc API: [%s]\n", baseUri);
//     context->SetWebService(OrthancStone::WasmWebService::GetInstance());
//     context->SetDelayedCallExecutor(OrthancStone::WasmDelayedCallExecutor::GetInstance());
//     application->Initialize(context.get(), statusBar_, parameters);
//     application->InitializeWasm();

// //    viewport->SetSize(width_, height_);
//     printf("StartWasmApplication - completed\n");
    SendFreeTextFromCppJS("Hello world from C++!");
}
