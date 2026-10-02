/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include "TypedObserver.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <list>

namespace OrthancStone
{
  namespace New
  {
    template <typename Message>
    class TypedObservable : public boost::noncopyable
    {
    private:
      typedef std::list< boost::weak_ptr< TypedObserver<Message> > >  Content;

      Content content_;

    public:
      void Register(const boost::shared_ptr< TypedObserver<Message> >& observer)
      {
        content_.push_back(observer);
      }

      void Notify(const IObservable& observable,
                  const Message& message)
      {
        Content active;

        for (typename Content::const_iterator it = content_.begin(); it != content_.end(); ++it)
        {
          boost::shared_ptr< TypedObserver<Message> > locked = it->lock();

          if (locked)
          {
            locked->Handle(observable, message);
            active.push_back(locked);
          }
        }

        content_.swap(active);
      }
    };
  }
}
