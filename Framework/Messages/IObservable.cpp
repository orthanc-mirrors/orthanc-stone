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


#include "IObservable.h"

#include <Core/OrthancException.h>

#include <cassert>

namespace OrthancStone 
{
  IObservable::~IObservable()
  {
    // delete all callables (this will also unregister them from the broker)
    for (Callables::const_iterator it = callables_.begin();
         it != callables_.end(); ++it)
    {
      for (std::set<ICallable*>::const_iterator
             it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
        delete *it2;
      }
    }

    // unregister the forwarders but don't delete them (they'll be
    // deleted by the observable they are observing as any other
    // callable)
    for (Forwarders::iterator it = forwarders_.begin();
         it != forwarders_.end(); ++it)
    {
      IMessageForwarder* fw = *it;
      broker_.Unregister(dynamic_cast<IObserver&>(*fw));
    }
  }
  

  void IObservable::RegisterObserverCallback(ICallable* callable)
  {
    if (callable == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    
    MessageType messageType = callable->GetMessageType();

    callables_[messageType].insert(callable);
  }

  
  void IObservable::EmitMessage(const IMessage& message)
  {
    Callables::const_iterator found = callables_.find(message.GetType());

    if (found != callables_.end())
    {
      for (std::set<ICallable*>::const_iterator
             it = found->second.begin(); it != found->second.end(); ++it)
      {
        assert(*it != NULL);
        if (broker_.IsActive(*(*it)->GetObserver()))
        {
          (*it)->Apply(message);
        }
      }
    }
  }

  
  void IObservable::RegisterForwarder(IMessageForwarder* forwarder)
  {
    if (forwarder == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    
    forwarders_.insert(forwarder);
  }
}