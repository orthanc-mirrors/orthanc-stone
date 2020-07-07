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


#pragma once

#include "../Messages/IMessage.h"
#include "OracleCommandBase.h"

namespace OrthancStone
{
  class SleepOracleCommand : public OracleCommandBase
  {
  private:
    unsigned int  milliseconds_;

  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, TimeoutMessage, SleepOracleCommand);

    SleepOracleCommand(unsigned int milliseconds) : 
      milliseconds_(milliseconds)
    {
    }

    virtual Type GetType() const
    {
      return Type_Sleep;
    }

    virtual IOracleCommand* Clone() const
    {
      return new SleepOracleCommand(milliseconds_);
    }

    unsigned int GetDelay() const
    {
      return milliseconds_;
    }
  };
}
