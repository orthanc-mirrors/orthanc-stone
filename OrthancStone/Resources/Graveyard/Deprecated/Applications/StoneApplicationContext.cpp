/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "StoneApplicationContext.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  void StoneApplicationContext::InitializeOrthanc()
  {
    if (webService_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    orthanc_.reset(new Deprecated::OrthancApiClient(*webService_, orthancBaseUrl_));
  }


  boost::shared_ptr<Deprecated::IWebService> StoneApplicationContext::GetWebService()
  {
    if (webService_ == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    
    return webService_;
  }

  
  boost::shared_ptr<Deprecated::OrthancApiClient> StoneApplicationContext::GetOrthancApiClient()
  {
    if (orthanc_.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    
    return orthanc_;
  }

  
  void StoneApplicationContext::SetWebService(boost::shared_ptr<Deprecated::IWebService> webService)
  {
    webService_ = webService;
    InitializeOrthanc();
  }


  void StoneApplicationContext::SetOrthancBaseUrl(const std::string& baseUrl)
  {
    // Make sure the base url ends with "/"
    if (baseUrl.empty() ||
        baseUrl[baseUrl.size() - 1] != '/')
    {
      orthancBaseUrl_ = baseUrl + "/";
    }
    else
    {
      orthancBaseUrl_ = baseUrl;
    }

    if (webService_ != NULL)
    {
      InitializeOrthanc();
    }
  }
}