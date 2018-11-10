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

#include "../StoneEnumerations.h"

#include <boost/noncopyable.hpp>

namespace OrthancStone 
{
  // base message that are exchanged between IObservable and IObserver
  class IMessage : public boost::noncopyable
  {
  private:
    int messageType_;
    
  protected:
    IMessage(const int& messageType) :
      messageType_(messageType)
    {
    }
    
  public:
    virtual ~IMessage()
    {
    }

    virtual int GetType() const
    {
      return messageType_;
    }
  };


  // base class to derive from to implement your own messages
  // it handles the message type for you
  template <int type>
  class BaseMessage : public IMessage
  {
  public:
    enum
    {
      Type = type
    };

    BaseMessage() :
      IMessage(static_cast<int>(Type))
    {
    }
  };
  

  // simple message implementation when no payload is needed
  // sample usage:
  // typedef NoPayloadMessage<MessageType_VolumeSlicer_GeometryReady> GeometryReadyMessage;
  template <int type>
  class NoPayloadMessage : public BaseMessage<type>
  {
  public:
    NoPayloadMessage() :
      BaseMessage<type>()
    {
    }
  };

  // simple message implementation when no payload is needed but the origin is required
  // sample usage:
  // typedef OriginMessage<MessageType_SliceLoader_GeometryError, OrthancSlicesLoader> SliceGeometryErrorMessage;
  template <int type, typename TOrigin>
  class OriginMessage : public BaseMessage<type>
  {
  private:
    TOrigin& origin_;

  public:
    OriginMessage(TOrigin& origin) :
      BaseMessage<type>(),
      origin_(origin)
    {
    }

    TOrigin& GetOrigin() const
    {
      return origin_;
    }
  };
}
