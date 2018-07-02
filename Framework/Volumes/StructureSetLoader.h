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

#include "../Toolbox/DicomStructureSet.h"
#include "../Toolbox/IWebService.h"
#include "VolumeLoaderBase.h"

namespace OrthancStone
{
  class StructureSetLoader :
    public VolumeLoaderBase,
    private IWebService::ICallback
  {
  private:
    class Operation;
    
    virtual void OnHttpRequestError(const std::string& uri,
                             Orthanc::IDynamicObject* payload);

    virtual void OnHttpRequestSuccess(const std::string& uri,
                               const void* answer,
                               size_t answerSize,
                               Orthanc::IDynamicObject* payload);

    IWebService&                      orthanc_;
    std::auto_ptr<DicomStructureSet>  structureSet_;

  public:
    StructureSetLoader(MessageBroker& broker, IWebService& orthanc);

    void ScheduleLoadInstance(const std::string& instance);

    bool HasStructureSet() const
    {
      return structureSet_.get() != NULL;
    }

    DicomStructureSet& GetStructureSet();
  };
}
