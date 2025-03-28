/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include "IOrthancConnection.h"

#if ORTHANC_ENABLE_CURL == 1

#include "../../Resources/Orthanc/Core/WebServiceParameters.h"

namespace OrthancStone
{
  class CurlOrthancConnection : public IOrthancConnection
  {
  private:
    Orthanc::WebServiceParameters  parameters_;

  public:
    CurlOrthancConnection(const Orthanc::WebServiceParameters& parameters) :
      parameters_(parameters)
    {
    }

    const Orthanc::WebServiceParameters& GetParameters() const
    {
      return parameters_;
    }

    virtual void RestApiGet(std::string& result,
                            const std::string& uri);

    virtual void RestApiPost(std::string& result,
                             const std::string& uri,
                             const std::string& body);
  };
}

#endif
