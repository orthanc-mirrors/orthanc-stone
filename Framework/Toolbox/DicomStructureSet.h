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


#pragma once

#include "CoordinateSystem3D.h"
#include "../Viewport/CairoContext.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/IOrthancConnection.h"

#include <list>

namespace OrthancStone
{
  class DicomStructureSet : public boost::noncopyable
  {
  private:
    typedef std::list<Vector>  Points;

    struct Polygon
    {
      double  projectionAlongNormal_;
      double  sliceThickness_;  // In millimeters
      Points  points_;
    };

    typedef std::list<Polygon>  Polygons;

    struct Structure
    {
      std::string   name_;
      std::string   interpretation_;
      Polygons      polygons_;
      uint8_t       red_;
      uint8_t       green_;
      uint8_t       blue_;
    };

    typedef std::vector<Structure>  Structures;

    Structures   structures_;
    std::string  parentSeriesId_;
    Vector       normal_;

    CoordinateSystem3D ExtractSliceGeometry(double& sliceThickness,
                                            OrthancPlugins::IOrthancConnection& orthanc,
                                            const OrthancPlugins::IDicomDataset& tags,
                                            size_t contourIndex,
                                            size_t sliceIndex);

    const Structure& GetStructure(size_t index) const;

    bool IsPolygonOnSlice(const Polygon& polygon,
                          const CoordinateSystem3D& geometry) const;


  public:
    DicomStructureSet(OrthancPlugins::IOrthancConnection& orthanc,
                      const std::string& instanceId);

    size_t GetStructureCount() const
    {
      return structures_.size();
    }

    Vector GetStructureCenter(size_t index) const;

    const std::string& GetStructureName(size_t index) const;

    const std::string& GetStructureInterpretation(size_t index) const;

    void GetStructureColor(uint8_t& red,
                           uint8_t& green,
                           uint8_t& blue,
                           size_t index) const;

    const Vector& GetNormal() const
    {
      return normal_;
    }

    void Render(CairoContext& context,
                const CoordinateSystem3D& slice) const;
  };
}
