/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017 Osimis, Belgium
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

#include "../../Resources/Orthanc/Core/IDynamicObject.h"

#include <string>

namespace OrthancStone
{
  class IWebService : public boost::noncopyable
  {
  public:
    class IRequestObserver : public boost::noncopyable
    {
    public:
      virtual ~IRequestObserver()
      {
      }

      virtual void NotifyError(Orthanc::IDynamicObject* payload) = 0;

      virtual void NotifyAnswer(const std::string& answer,
                                Orthanc::IDynamicObject* payload) = 0;
    };
    
    virtual ~IWebService()
    {
    }

    virtual void ScheduleGetRequest(IRequestObserver& observer,
                                    const std::string& uri,
                                    Orthanc::IDynamicObject* payload) = 0;

    virtual void SchedulePostRequest(IRequestObserver& observer,
                                     const std::string& uri,
                                     const std::string& body,
                                     Orthanc::IDynamicObject* payload) = 0;
  };
}
