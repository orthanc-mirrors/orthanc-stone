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

#include "../Messages/IMessage.h"
#include "OracleCommandBase.h"

namespace OrthancStone
{
  class ReadFileCommand : public OracleCommandBase
  {
  public:
    class SuccessMessage : public OracleMessageBase
    {
      ORTHANC_STONE_MESSAGE(__FILE__, __LINE__);
      
    private:
      std::string content_;

    public:
      SuccessMessage(ReadFileCommand& command,
                     std::string& content  /* will be swapped to avoid a memcpy() */) :
        OracleMessageBase(command)
      {
        content_.swap(content);
      }

      const std::string& GetContent() const
      {
        return content_;
      }
    };


  private:
    std::string  path_;

  public:
    ReadFileCommand(const std::string& path) : 
      path_(path)
    {
    }

    virtual Type GetType() const
    {
      return Type_ReadFile;
    }

    const std::string& GetPath() const
    {
      return path_;
    }
  };
}
