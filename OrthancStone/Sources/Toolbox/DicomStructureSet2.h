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

#ifdef BGO_ENABLE_DICOMSTRUCTURESETLOADER2

#include "DicomStructure2.h"
#include "CoordinateSystem3D.h"
#include "Extent2D.h"
#include "../Scene2D/Color.h"

#include <FullOrthancDataset.h>

#include <list>

namespace OrthancStone
{
  class DicomStructureSet2 : public boost::noncopyable
  {
  public:
    DicomStructureSet2();
    ~DicomStructureSet2();
   
    void SetContents(const OrthancPlugins::FullOrthancDataset& tags);

    size_t GetStructuresCount() const
    {
      return structures_.size();
    }

    void Clear();

    const DicomStructure2& GetStructure(size_t i) const
    {
      // at() is like []() but with range check
      return structures_.at(i);
    }

    /** Internal use only */
    void FillStructuresFromDataset(const OrthancPlugins::FullOrthancDataset& tags);

    /** Internal use only */
    void ComputeDependentProperties();

    /** Internal use only */
    std::vector<DicomStructure2> structures_;
  };
}

#endif 
// BGO_ENABLE_DICOMSTRUCTURESETLOADER2


