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

#include "IWebService.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/IOrthancConnection.h"
#include "../../Resources/Orthanc/Core/WebServiceParameters.h"

#include <memory>

// TODO REMOVE THIS

namespace OrthancStone
{
  class OrthancSynchronousWebService : public IWebService
  {
  private:
    std::auto_ptr<OrthancPlugins::IOrthancConnection>  orthanc_;
    
  public:
    OrthancSynchronousWebService(OrthancPlugins::IOrthancConnection* orthanc);  // Takes ownership
    
    OrthancSynchronousWebService(const Orthanc::WebServiceParameters& parameters);

    OrthancPlugins::IOrthancConnection& GetConnection()
    {
      return *orthanc_;
    }
    
    virtual void ScheduleGetRequest(ICallback& callback,
                                    const std::string& uri,
                                    Orthanc::IDynamicObject* payload);

    virtual void SchedulePostRequest(ICallback& callback,
                                     const std::string& uri,
                                     const std::string& body,
                                     Orthanc::IDynamicObject* payload);
  };
}
