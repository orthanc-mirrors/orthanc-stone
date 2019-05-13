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
    
    const MessageIdentifier& id = callable->GetMessageIdentifier();
    callables_[id].insert(callable);
  }

  void IObservable::Unregister(IObserver *observer)
  {
    // delete all callables from this observer
    for (Callables::iterator itCallableSet = callables_.begin();
         itCallableSet != callables_.end(); ++itCallableSet)
    {
      for (std::set<ICallable*>::const_iterator
             itCallable = itCallableSet->second.begin(); itCallable != itCallableSet->second.end(); )
      {
        if ((*itCallable)->GetObserver() == observer)
        {
          delete *itCallable;
          itCallableSet->second.erase(itCallable++);
        }
        else
          ++itCallable;
      }
    }
  }
  
  void IObservable::EmitMessageInternal(const IObserver* receiver,
                                        const IMessage& message)
  {
    Callables::const_iterator found = callables_.find(message.GetIdentifier());

    if (found != callables_.end())
    {
      for (std::set<ICallable*>::const_iterator
             it = found->second.begin(); it != found->second.end(); ++it)
      {
        assert(*it != NULL);

        const IObserver* observer = (*it)->GetObserver();
        if (broker_.IsActive(*observer))
        {
          if (receiver == NULL ||    // Are we broadcasting?
              observer == receiver)  // Not broadcasting, but this is the receiver
          {
            (*it)->Apply(message);
          }
        }
      }
    }
  }


  void IObservable::BroadcastMessage(const IMessage& message)
  {
    EmitMessageInternal(NULL, message);
  }

  
  void IObservable::EmitMessage(const IObserver& observer,
                                const IMessage& message)
  {
    EmitMessageInternal(&observer, message);
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
