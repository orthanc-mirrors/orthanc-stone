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


#include "OrthancSynchronousWebService.h"

#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/OrthancHttpConnection.h"

namespace OrthancStone
{
  OrthancSynchronousWebService::OrthancSynchronousWebService(OrthancPlugins::IOrthancConnection* orthanc) :
    orthanc_(orthanc)
  {
    if (orthanc == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
  }
  
  OrthancSynchronousWebService::OrthancSynchronousWebService(const Orthanc::WebServiceParameters& parameters)
  {
    orthanc_.reset(new OrthancPlugins::OrthancHttpConnection(parameters));
  }    

  void OrthancSynchronousWebService::ScheduleGetRequest(ICallback& callback,
                                             const std::string& uri,
                                             Orthanc::IDynamicObject* payload)
  {
    std::auto_ptr<Orthanc::IDynamicObject> tmp(payload);

    try
    {
      std::string answer;
      orthanc_->RestApiGet(answer, uri);
      callback.NotifySuccess(uri, answer.c_str(), answer.size(), tmp.release());
    }
    catch (Orthanc::OrthancException&)
    {
      callback.NotifyError(uri, tmp.release());
    }
  }

  void OrthancSynchronousWebService::SchedulePostRequest(ICallback& callback,
                                              const std::string& uri,
                                              const std::string& body,
                                              Orthanc::IDynamicObject* payload)
  {
    std::auto_ptr<Orthanc::IDynamicObject> tmp(payload);

    try
    {
      std::string answer;
      orthanc_->RestApiPost(answer, uri, body);
      callback.NotifySuccess(uri, answer.c_str(), answer.size(), tmp.release());
    }
    catch (Orthanc::OrthancException&)
    {
      callback.NotifyError(uri, tmp.release());
    }
  }
}
