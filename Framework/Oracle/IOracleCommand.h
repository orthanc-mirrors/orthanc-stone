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

#include <Core/IDynamicObject.h>

#include <boost/noncopyable.hpp>

namespace OrthancStone
{
  class IOracleCommand : public boost::noncopyable
  {
  public:
    class OracleMessageBase : public IMessage
    {
    private:
      IOracleCommand&  command_;

    public:
      OracleMessageBase(IOracleCommand& command) :
        command_(command)
      {
      }

      void AcquireCommandPayload(Orthanc::IDynamicObject* payload) const
      {
        command_.AcquirePayload(payload);
      }

      const IOracleCommand& GetCommand() const
      {
        return command_;
      }
    };

    enum Type
    {
      Type_GetOrthancImage,
      Type_GetOrthancWebViewerJpeg,
      Type_Http,
      Type_OrthancRestApi,
      Type_ParseDicomFile,
      Type_ReadFile,
      Type_Sleep
    };

    virtual ~IOracleCommand()
    {
    }

    virtual Type GetType() const = 0;

    virtual void AcquirePayload(Orthanc::IDynamicObject* payload) = 0;
    
    virtual bool HasPayload() const = 0;

    virtual Orthanc::IDynamicObject& GetPayload() const = 0;
  };
}
