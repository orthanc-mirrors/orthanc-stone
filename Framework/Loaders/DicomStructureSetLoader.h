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

#include "../Toolbox/DicomStructureSet.h"
#include "../Volumes/IVolumeSlicer.h"
#include "LoaderStateMachine.h"

namespace OrthancStone
{
  class DicomStructureSetLoader :
    public LoaderStateMachine,
    public IVolumeSlicer,
    public IObservable
  {
  private:
    class Slice;

    // States of LoaderStateMachine
    class AddReferencedInstance;   // 3rd state
    class LookupInstance;          // 2nd state
    class LoadStructure;           // 1st state
    
    std::auto_ptr<DicomStructureSet>  content_;
    uint64_t                          revision_;
    std::string                       instanceId_;
    unsigned int                      countProcessedInstances_;
    unsigned int                      countReferencedInstances_;  

    // will be set to true once the loading is finished
    bool                              structuresReady_;
    
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, StructuresReady, DicomStructureSetLoader);

    DicomStructureSetLoader(IOracle& oracle,
                            IObservable& oracleObservable);    
    
    ~DicomStructureSetLoader();
    
    void LoadInstance(const std::string& instanceId);

    virtual IExtractedSlice* ExtractSlice(const CoordinateSystem3D& cuttingPlane);

    void SetStructuresReady();

    bool AreStructuresReady() const;
  };
}
