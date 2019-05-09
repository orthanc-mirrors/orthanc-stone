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

#include "PointerEvent.h"

namespace OrthancStone
{
  class IPointerTracker : public boost::noncopyable
  {
  public:
    virtual ~IPointerTracker()
    {
    }

    /**
    This method will be repeatedly called during user interaction
    */
    virtual void Update(const PointerEvent& event) = 0;

    /**
    This method will be called if the tracker is to be abandoned without
    committing its result
    */
    virtual void Release() = 0;
  };
}
