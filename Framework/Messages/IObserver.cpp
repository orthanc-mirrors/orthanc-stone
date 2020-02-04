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


#include "IObserver.h"

#include "IMessage.h"
#include "../StoneException.h"

#include <Core/Logging.h>
#include <Core/Toolbox.h>

namespace OrthancStone 
{
  static const uint64_t IObserver_FIRST_UNIQUE_ID = 10973;
  static uint64_t IObserver_nextUniqueId = IObserver_FIRST_UNIQUE_ID;
  
  IObserver::IObserver(MessageBroker& broker)
    : broker_(broker)
  {
    AssignFingerprint();
    broker_.Register(*this);
  }

  IObserver::~IObserver()
  {
    try
    {
      LOG(TRACE) << "IObserver(" << std::hex << this << std::dec << ")::~IObserver : fingerprint_[0] == " << fingerprint_[0];
      fingerprint_[0] = 0xdeadbeef;
      fingerprint_[1] = 0xdeadbeef;
      fingerprint_[2] = 0xdeadbeef;
      broker_.Unregister(*this);
    }
    catch (const Orthanc::OrthancException& e)
    {
      if (e.HasDetails())
      {
        LOG(ERROR) << "OrthancException in ~IObserver: " << e.What() << " Details: " << e.GetDetails();
      }
      else
      {
        LOG(ERROR) << "OrthancException in ~IObserver: " << e.What();
      }
    }
    catch (const std::exception& e)
    {
      LOG(ERROR) << "std::exception in ~IObserver: " << e.what();
    }
    catch (...)
    {
      LOG(ERROR) << "Unknown exception in ~IObserver";
    }
  }

  static const int64_t IObserver_UNIQUE_ID_MAGIC_NUMBER = 2742024;

  void IObserver::AssignFingerprint()
  {
    fingerprint_[0] = IObserver_nextUniqueId;
    fingerprint_[1] = fingerprint_[0] / 2;
    fingerprint_[2] = fingerprint_[1] + IObserver_UNIQUE_ID_MAGIC_NUMBER;
    IObserver_nextUniqueId++;
  }

  bool IObserver::DoesFingerprintLookGood() const
  {
    bool ok = (fingerprint_[0] >= IObserver_FIRST_UNIQUE_ID) &&
      (fingerprint_[1] == fingerprint_[0] / 2) &&
      (fingerprint_[2] == fingerprint_[1] + IObserver_UNIQUE_ID_MAGIC_NUMBER);
    if(!ok) 
    {
      LOG(INFO) << "Fingerprint not valid: " << " fingerprint_[0] = " << fingerprint_[0] << " fingerprint_[1] = " << fingerprint_[1]<< " fingerprint_[2] = " << fingerprint_[2];
    }
    return ok;
  }
}
