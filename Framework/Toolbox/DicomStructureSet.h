/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

#include "SliceGeometry.h"
#include "../Viewport/CairoContext.h"

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

    SliceGeometry ExtractSliceGeometry(double& sliceThickness,
                                       IOrthancConnection& orthanc,
                                       const Json::Value& contour);

    const Structure& GetStructure(size_t index) const;

    bool IsPolygonOnSlice(const Polygon& polygon,
                          const SliceGeometry& geometry) const;


  public:
    DicomStructureSet(IOrthancConnection& orthanc,
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
                const SliceGeometry& slice) const;
  };
}
