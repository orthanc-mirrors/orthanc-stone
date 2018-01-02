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


#include "Enumerations.h"

#include <Core/Logging.h>

namespace OrthancStone
{  
  bool StringToSopClassUid(SopClassUid& result,
                           const std::string& source)
  {
    if (source == "1.2.840.10008.5.1.4.1.1.481.2")
    {
      result = SopClassUid_RTDose;
      return true;
    }
    else
    {
      //LOG(INFO) << "Unknown SOP class UID: " << source;
      return false;
    }
  }  
}
