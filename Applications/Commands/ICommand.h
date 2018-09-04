/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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


#pragma once

#include <json/json.h>

// TODO: must be reworked completely (check trello)

namespace OrthancStone
{
  class ICommand  // TODO noncopyable
  {
  protected:
    const char* name_;
    ICommand(const char* name)
      : name_(name)
    {}
  public:
    virtual void Execute() = 0;
    virtual void Configure(const Json::Value& arguments) = 0;
    const char* GetName()
    {
      return name_;
    }
  };


  template <typename TCommand>
  class BaseCommand : ICommand
  {
  protected:
    BaseCommand(const char* name)
      : ICommand(name)
    {}

  public:
    static ICommand* Create() {
      return new TCommand();
    }

    virtual void Configure(const Json::Value& arguments) {
    }
  };

  class NoopCommand : public BaseCommand<NoopCommand>
  {
  public:
    NoopCommand()
      : BaseCommand("noop")
    {}
    virtual void Execute() {}
  };


}
