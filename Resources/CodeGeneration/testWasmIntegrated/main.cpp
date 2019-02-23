#include <iostream>
#include <emscripten/emscripten.h>

int main()
{
    std::cout << "Hello world from testWasmIntegrated!" << std::endl;
}

void EMSCRIPTEN_KEEPALIVE StartWasmApplication(const char* baseUri)
{
    printf("StartWasmApplication\n");

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
  }
  
