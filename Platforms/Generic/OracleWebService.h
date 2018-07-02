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

#include "../../Framework/Toolbox/IWebService.h"
#include "Oracle.h"
#include "WebServiceGetCommand.h"
#include "WebServicePostCommand.h"

namespace OrthancStone
{
  class OracleWebService : public IWebService
  {
  private:
    Oracle&                        oracle_;
    Orthanc::WebServiceParameters  parameters_;

  public:
    OracleWebService(MessageBroker& broker,
                     Oracle& oracle,
                     const Orthanc::WebServiceParameters& parameters) : 
      IWebService(broker),
      oracle_(oracle),
      parameters_(parameters)
    {
    }

    virtual void ScheduleGetRequest(ICallback& callback,
                                    const std::string& uri,
                                    Orthanc::IDynamicObject* payload)
    {
      oracle_.Submit(new WebServiceGetCommand(broker_, callback, parameters_, uri, payload));
    }

    virtual void SchedulePostRequest(ICallback& callback,
                                     const std::string& uri,
                                     const std::string& body,
                                     Orthanc::IDynamicObject* payload)
    {
      oracle_.Submit(new WebServicePostCommand(broker_, callback, parameters_, uri, body, payload));
    }

    void Start()
    {
        oracle_.Start();
    }

    void Stop()
    {
        oracle_.Stop();
    }
  };
}
