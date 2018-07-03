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


#include "MessageBroker.h"

#include <algorithm>
#include <assert.h>
#include <vector>

#include "IObserver.h"
#include "MessageType.h"

namespace OrthancStone {

  void MessageBroker::Emit(IObservable& from, std::set<IObserver*> observers, const IMessage& message)
  {
    std::vector<IObserver*> activeObservers;
    std::set_intersection(observers.begin(),
                          observers.end(),
                          activeObservers_.begin(),
                          activeObservers_.end(),
                          std::back_inserter(activeObservers)
                          );

    for (std::vector<IObserver*>::iterator observer = activeObservers.begin(); observer != activeObservers.end(); observer++)
    {
      (*observer)->HandleMessage(from, message);
    }
  }

}
