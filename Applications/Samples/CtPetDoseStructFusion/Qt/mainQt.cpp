#include "Applications/Qt/QtStoneApplicationRunner.h"

#include "../CtPetDoseStructFusionApplication.h"
#include "Framework/Messages/MessageBroker.h"


int main(int argc, char* argv[]) 
{
  OrthancStone::MessageBroker broker;
  CtPetDoseStructFusion::CtPetDoseStructFusionApplication stoneApplication(broker);

  OrthancStone::QtStoneApplicationRunner qtAppRunner(broker, stoneApplication);
  return qtAppRunner.Execute(argc, argv);
}
