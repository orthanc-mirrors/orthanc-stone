/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "OrthancPluginConnection.h"

#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include <OrthancException.h>

namespace OrthancStone
{
  void OrthancPluginConnection::RestApiGet(std::string& result,
                                           const std::string& uri)
  {
    OrthancPlugins::MemoryBuffer tmp;
    
    if (tmp.RestApiGet(uri, false))
    {
      tmp.ToString(result);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
    }
  }

  
  void OrthancPluginConnection::RestApiPost(std::string& result,
                                            const std::string& uri,
                                            const std::string& body)
  {
    OrthancPlugins::MemoryBuffer tmp;
    
    if (tmp.RestApiPost(uri, body.empty() ? NULL : body.c_str(), body.size(), false))
    {
      tmp.ToString(result);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
    }
  }
   

  void OrthancPluginConnection::RestApiPut(std::string& result,
                                           const std::string& uri,
                                           const std::string& body)
  {
    OrthancPlugins::MemoryBuffer tmp;
    
    if (tmp.RestApiPut(uri, body.empty() ? NULL : body.c_str(), body.size(), false))
    {
      tmp.ToString(result);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
    }
  }

  
  void OrthancPluginConnection::RestApiDelete(const std::string& uri)
  {
    if (!OrthancPlugins::RestApiDelete(uri, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);
    }   
  }
}