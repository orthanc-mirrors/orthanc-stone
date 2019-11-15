/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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

#include "OracleCommandBase.h"

namespace OrthancStone
{
  class ParseDicomFromWadoCommand : public OracleCommandBase
  {
  private:
    std::string                    sopInstanceUid_;
    std::auto_ptr<IOracleCommand>  restCommand_;

  public:
    ParseDicomFromWadoCommand(const std::string& sopInstanceUid,
                              IOracleCommand* restCommand);

    virtual Type GetType() const
    {
      return Type_ParseDicomFromWado;
    }

    virtual IOracleCommand* Clone() const;

    const std::string& GetSopInstanceUid() const
    {
      return sopInstanceUid_;
    }
    
    const IOracleCommand& GetRestCommand() const;
  };
}
