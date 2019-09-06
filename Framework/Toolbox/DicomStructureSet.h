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

#include "CoordinateSystem3D.h"
#include "Extent2D.h"
#include "../Scene2D/Color.h"

#include <Plugins/Samples/Common/FullOrthancDataset.h>

#include <list>

namespace OrthancStone
{
  class DicomStructureSet : public boost::noncopyable
  {
  public:
    typedef std::pair<double, double> PolygonPoint2D;
    
  private:
    struct ReferencedSlice
    {
      std::string          seriesInstanceUid_;
      CoordinateSystem3D   geometry_;
      double               thickness_;

      ReferencedSlice()
      {
      }
      
      ReferencedSlice(const std::string& seriesInstanceUid,
                      const CoordinateSystem3D& geometry,
                      double thickness) :
        seriesInstanceUid_(seriesInstanceUid),
        geometry_(geometry),
        thickness_(thickness)
      {
      }
    };

    typedef std::map<std::string, ReferencedSlice>  ReferencedSlices;
    
    typedef std::vector<Vector>  Points;

    class Polygon
    {
    private:
      std::string         sopInstanceUid_;
      bool                hasSlice_;
      CoordinateSystem3D  geometry_;
      double              projectionAlongNormal_;
      double              sliceThickness_;  // In millimeters
      Points              points_;
      Extent2D            extent_;

      void CheckPointIsOnSlice(const Vector& v) const;
      bool IsPointOnSlice(const Vector& v) const;

    public:
      Polygon(const std::string& sopInstanceUid) :
        sopInstanceUid_(sopInstanceUid),
        hasSlice_(false)
      {
      }

      void Reserve(size_t n)
      {
        points_.reserve(n);
      }

      void AddPoint(const Vector& v);

      bool UpdateReferencedSlice(const ReferencedSlices& slices);

      bool IsOnSlice(const CoordinateSystem3D& geometry) const;

      const std::string& GetSopInstanceUid() const
      {
        return sopInstanceUid_;
      }

      const Points& GetPoints() const
      {
        return points_;
      }

      double GetSliceThickness() const
      {
        return sliceThickness_;
      }

      bool Project(double& x1,
                   double& y1,
                   double& x2,
                   double& y2,
                   const CoordinateSystem3D& slice) const;
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

    Structures        structures_;
    ReferencedSlices  referencedSlices_;

    const Structure& GetStructure(size_t index) const;

    Structure& GetStructure(size_t index);
  
    bool ProjectStructure(std::vector< std::vector<PolygonPoint2D> >& polygons,
                          const Structure& structure,
                          const CoordinateSystem3D& slice) const;

  public:
    DicomStructureSet(const OrthancPlugins::FullOrthancDataset& instance);

    size_t GetStructuresCount() const
    {
      return structures_.size();
    }

    Vector GetStructureCenter(size_t index) const;

    const std::string& GetStructureName(size_t index) const;

    const std::string& GetStructureInterpretation(size_t index) const;

    Color GetStructureColor(size_t index) const;

    // TODO - remove
    void GetStructureColor(uint8_t& red,
                           uint8_t& green,
                           uint8_t& blue,
                           size_t index) const;

    void GetReferencedInstances(std::set<std::string>& instances);

    void AddReferencedSlice(const std::string& sopInstanceUid,
                            const std::string& seriesInstanceUid,
                            const CoordinateSystem3D& geometry,
                            double thickness);

    void AddReferencedSlice(const Orthanc::DicomMap& dataset);

    void CheckReferencedSlices();

    Vector GetNormal() const;

    bool ProjectStructure(std::vector< std::vector<PolygonPoint2D> >& polygons,
                          size_t index,
                          const CoordinateSystem3D& slice) const
    {
      return ProjectStructure(polygons, GetStructure(index), slice);
    }
  };
}
