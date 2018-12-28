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
#include "ICallable.h"
#include "IMessage.h"

#include <boost/noncopyable.hpp>
#include <memory>

namespace OrthancStone {

  class Promise : public boost::noncopyable
  {
  protected:
    MessageBroker&                    broker_;

    std::auto_ptr<ICallable> successCallable_;
    std::auto_ptr<ICallable> failureCallable_;

  public:
    Promise(MessageBroker& broker)
      : broker_(broker)
    {
    }

    void Success(const IMessage& message)
    {
      // check the target is still alive in the broker
      if (successCallable_.get() != NULL &&
          broker_.IsActive(*successCallable_->GetObserver()))
      {
        successCallable_->Apply(message);
      }
    }

    void Failure(const IMessage& message)
    {
      // check the target is still alive in the broker
      if (failureCallable_.get() != NULL &&
          broker_.IsActive(*failureCallable_->GetObserver()))
      {
        failureCallable_->Apply(message);
      }
    }

    Promise& Then(ICallable* successCallable)   // Takes ownership
    {
      if (successCallable_.get() != NULL)
      {
        // TODO: throw throw new "Promise may only have a single success target"
      }
      successCallable_.reset(successCallable);
      return *this;
    }

    Promise& Else(ICallable* failureCallable)   // Takes ownership
    {
      if (failureCallable_.get() != NULL)
      {
        // TODO: throw throw new "Promise may only have a single failure target"
      }
      failureCallable_.reset(failureCallable);
      return *this;
    }
  };
}
