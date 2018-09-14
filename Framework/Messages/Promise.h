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

#include "MessageBroker.h"
#include "ICallable.h"
#include "IMessage.h"

#include <boost/noncopyable.hpp>

namespace OrthancStone {

  class Promise : public boost::noncopyable
  {
  protected:
    MessageBroker&                    broker_;

    ICallable* successCallable_;
    ICallable* failureCallable_;

  public:
    Promise(MessageBroker& broker)
      : broker_(broker),
        successCallable_(NULL),
        failureCallable_(NULL)
    {
    }

    void Success(const IMessage& message)
    {
      // check the target is still alive in the broker
      if (broker_.IsActive(successCallable_->GetObserver()))
      {
        successCallable_->Apply(message);
      }
    }

    void Failure(const IMessage& message)
    {
      // check the target is still alive in the broker
      if (broker_.IsActive(failureCallable_->GetObserver()))
      {
        failureCallable_->Apply(message);
      }
    }

    Promise& Then(ICallable* successCallable)
    {
      if (successCallable_ != NULL)
      {
        // TODO: throw throw new "Promise may only have a single success target"
      }
      successCallable_ = successCallable;
      return *this;
    }

    Promise& Else(ICallable* failureCallable)
    {
      if (failureCallable_ != NULL)
      {
        // TODO: throw throw new "Promise may only have a single failure target"
      }
      failureCallable_ = failureCallable;
      return *this;
    }

  };


}
