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

#include "MessageBroker.h"
#include "IMessage.h"

#include <Core/Toolbox.h>

namespace OrthancStone 
{
  class IObserver : public boost::noncopyable
  {
  private:
    MessageBroker&  broker_;
    // the following is a UUID that is used to disambiguate different observers
    // that may have the same address
    char     fingerprint_[37];
  public:
    IObserver(MessageBroker& broker)
      : broker_(broker)
      , fingerprint_()
    {
      // we store the fingerprint_ as a char array to avoid problems when
      // reading it in a deceased object.
      // remember this is panic-level code to track zombie object usage
      std::string fingerprint = Orthanc::Toolbox::GenerateUuid();
      const char* fingerprintRaw = fingerprint.c_str();
      memcpy(fingerprint_, fingerprintRaw, 37);
      broker_.Register(*this);
    }

    virtual ~IObserver()
    {
      try
      {
        LOG(TRACE) << "IObserver(" << std::hex << this << std::dec << ")::~IObserver : fingerprint_ == " << fingerprint_;
        const char* deadMarker = "deadbeef-dead-dead-0000-0000deadbeef";
        ORTHANC_ASSERT(strlen(deadMarker) == 36);
        memcpy(fingerprint_, deadMarker, 37);
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

    const char* GetFingerprint() const
    {
      return fingerprint_;
    }

    bool DoesFingerprintLookGood() const
    {
      for (size_t i = 0; i < 36; ++i) {
        bool ok = false;
        if (fingerprint_[i] >= 'a' && fingerprint_[i] <= 'f')
          ok = true;
        if (fingerprint_[i] >= '0' && fingerprint_[i] <= '9')
          ok = true;
        if (fingerprint_[i] == '-')
          ok = true;
        if (!ok)
          return false;
      }
      return fingerprint_[36] == 0;
    }

    MessageBroker& GetBroker() const
    {
      return broker_;
    }
  };
}
