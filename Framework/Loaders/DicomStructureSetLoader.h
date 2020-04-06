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


#pragma once

#include "../Toolbox/DicomStructureSet.h"
#include "../Volumes/IVolumeSlicer.h"
#include "../Loaders/ILoadersContext.h"
#include "LoaderStateMachine.h"

#include <vector>

namespace OrthancStone
{
  class DicomStructureSetLoader :
    public LoaderStateMachine,
    public OrthancStone::IVolumeSlicer,
    public OrthancStone::IObservable
  {
  private:
    class Slice;

    // States of LoaderStateMachine
    class AddReferencedInstance;   // 3rd state
    class LookupInstance;          // 2nd state
    class LoadStructure;           // 1st state
    
    OrthancStone::ILoadersContext&                    loadersContext_;
    std::unique_ptr<OrthancStone::DicomStructureSet>  content_;
    uint64_t                                          revision_;
    std::string                                       instanceId_;
    unsigned int                                      countProcessedInstances_;
    unsigned int                                      countReferencedInstances_;  

    // will be set to true once the loading is finished
    bool                                              structuresReady_;

    /**
    At load time, these strings are used to initialize the structureVisibility_ 
    vector.

    As a special case, if initiallyVisibleStructures_ contains a single string
    that is '*', ALL structures will be made visible.
    */
    std::vector<std::string> initiallyVisibleStructures_;

    /**
    Contains the "Should this structure be displayed?" flag for all structures.
    Only filled when structures are loaded.

    Changing this value directly affects the rendering
    */
    std::vector<bool>                  structureVisibility_;

  protected:
    DicomStructureSetLoader(OrthancStone::ILoadersContext& loadersContext);

  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, StructuresReady, DicomStructureSetLoader);
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, StructuresUpdated, DicomStructureSetLoader);

    static boost::shared_ptr<DicomStructureSetLoader> Create(
      OrthancStone::ILoadersContext& loadersContext);

    OrthancStone::DicomStructureSet* GetContent()
    {
      return content_.get();
    }

    void SetStructureDisplayState(size_t structureIndex, bool display);
    
    bool GetStructureDisplayState(size_t structureIndex) const
    {
      return structureVisibility_.at(structureIndex);
    }

    ~DicomStructureSetLoader();
    
    void LoadInstance(const std::string& instanceId, 
                      const std::vector<std::string>& initiallyVisibleStructures = std::vector<std::string>());

    virtual IExtractedSlice* ExtractSlice(const OrthancStone::CoordinateSystem3D& cuttingPlane) ORTHANC_OVERRIDE;

    void SetStructuresReady();
    void SetStructuresUpdated();

    bool AreStructuresReady() const;
  };
}
