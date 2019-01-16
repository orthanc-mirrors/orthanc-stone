#include <iostream>
#include "basic_generated.h"

using namespace Orthanc::Stone::Samples::SimpleViewer;

namespace OrthancStone
{
    class IPocMessage
    {
    public:
        virtual std::string GetType() = 0;
    };


}



int main()
{
    Request;

    Request::messageType = SelectTool;

    SelectToolT tool2;
    tool2.tool = Tool_CircleMeasure;
    
    SendToApplication(tool2);
    

    tool2.tool = Tool_CircleMeasure;

    CircleToolT circleTool;
    circleTool.centerX = 3;
    circleTool.centerY = 4;

    std::cout << "Hello from flatc generator sample!" << std::endl;
}
