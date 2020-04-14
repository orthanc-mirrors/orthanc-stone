#include "Applications/Qt/QtStoneApplicationRunner.h"

#include "../../SimpleViewerApplication.h"
#include "Framework/Messages/MessageBroker.h"


int main(int argc, char* argv[]) 
{
  OrthancStone::MessageBroker broker;
  SimpleViewer::SimpleViewerApplication stoneApplication(broker);

  OrthancStone::QtStoneApplicationRunner qtAppRunner(broker, stoneApplication);
  return qtAppRunner.Execute(argc, argv);
}
