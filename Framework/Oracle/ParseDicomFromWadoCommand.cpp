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


#include "ParseDicomFromWadoCommand.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  ParseDicomFromWadoCommand::ParseDicomFromWadoCommand(const std::string& sopInstanceUid,
                                                       IOracleCommand* restCommand) :
    sopInstanceUid_(sopInstanceUid),
    restCommand_(restCommand)
  {
    if (restCommand == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    if (restCommand_->GetType() != Type_Http &&
        restCommand_->GetType() != Type_OrthancRestApi)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
    }        
  }

  
  IOracleCommand* ParseDicomFromWadoCommand::Clone() const
  {
    assert(restCommand_.get() != NULL);
    return new ParseDicomFromWadoCommand(sopInstanceUid_, restCommand_->Clone());
  }


  const IOracleCommand& ParseDicomFromWadoCommand::GetRestCommand() const
  {
    assert(restCommand_.get() != NULL);
    return *restCommand_;
  }
}
