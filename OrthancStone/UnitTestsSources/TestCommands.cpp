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


#include <gtest/gtest.h>

//#include "../Applications/Commands/BaseCommandFactory.h"
//#include "OrthancException.h"

//class CommandIncrement: public OrthancStone::BaseCommand<CommandIncrement>
//{
//public:
//  static int counter;
//  int increment_;
//public:
//  CommandIncrement()
//    : OrthancStone::BaseCommand<CommandIncrement>("increment"),
//      increment_(0)
//  {}

//  virtual void Execute()
//  {
//    counter += increment_;
//  }
//  virtual void Configure(const Json::Value& arguments)
//  {
//    increment_ = arguments["increment"].asInt();
//  }
//};

//// COMMAND("name", "arg1", "int", "arg2", "string")
//// COMMAND(name, arg1, arg2)


//int CommandIncrement::counter = 0;

//TEST(Commands, CreateNoop)
//{
//  OrthancStone::BaseCommandFactory factory;

//  factory.RegisterCommandClass<OrthancStone::NoopCommand>();

//  Json::Value cmdJson;
//  cmdJson["command"] = "noop";

//  std::unique_ptr<OrthancStone::ICommand> command(factory.CreateFromJson(cmdJson));

//  ASSERT_TRUE(command.get() != NULL);
//  ASSERT_EQ("noop", command->GetName());
//}

//TEST(Commands, Execute)
//{
//  OrthancStone::BaseCommandFactory factory;

//  factory.RegisterCommandClass<OrthancStone::NoopCommand>();
//  factory.RegisterCommandClass<CommandIncrement>();

//  Json::Value cmdJson;
//  cmdJson["command"] = "increment";
//  cmdJson["args"]["increment"] = 2;

//  std::unique_ptr<OrthancStone::ICommand> command(factory.CreateFromJson(cmdJson));

//  ASSERT_TRUE(command.get() != NULL);
//  CommandIncrement::counter = 0;
//  command->Execute();
//  ASSERT_EQ(2, CommandIncrement::counter);
//}

//TEST(Commands, TryCreateUnknowCommand)
//{
//  OrthancStone::BaseCommandFactory factory;
//  factory.RegisterCommandClass<OrthancStone::NoopCommand>();

//  Json::Value cmdJson;
//  cmdJson["command"] = "unknown";

//  ASSERT_THROW(std::unique_ptr<OrthancStone::ICommand> command(factory.CreateFromJson(cmdJson)), Orthanc::OrthancException);
//}

//TEST(Commands, TryCreateCommandFromInvalidJson)
//{
//  OrthancStone::BaseCommandFactory factory;
//  factory.RegisterCommandClass<OrthancStone::NoopCommand>();

//  Json::Value cmdJson;
//  cmdJson["command-name"] = "noop";

//  ASSERT_THROW(std::unique_ptr<OrthancStone::ICommand> command(factory.CreateFromJson(cmdJson)), Orthanc::OrthancException);
//}
