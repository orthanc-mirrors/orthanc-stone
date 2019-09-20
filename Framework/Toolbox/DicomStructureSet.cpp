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


#include "DicomStructureSet.h"
#include "DicomStructureSetUtils.h"

#include "../Toolbox/GeometryToolbox.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4244)
#endif

#include <limits>
#include <stdio.h>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

typedef boost::geometry::model::d2::point_xy<double> BoostPoint;
typedef boost::geometry::model::polygon<BoostPoint> BoostPolygon;
typedef boost::geometry::model::multi_polygon<BoostPolygon>  BoostMultiPolygon;


static void Union(BoostMultiPolygon& output,
                  std::vector<BoostPolygon>& input)
{
  for (size_t i = 0; i < input.size(); i++)
  {
    boost::geometry::correct(input[i]);
  }
  
  if (input.size() == 0)
  {
    output.clear();
  }
  else if (input.size() == 1)
  {
    output.resize(1);
    output[0] = input[0];
  }
  else
  {
    boost::geometry::union_(input[0], input[1], output);

    for (size_t i = 0; i < input.size(); i++)
    {
      BoostMultiPolygon tmp;
      boost::geometry::union_(output, input[i], tmp);
      output = tmp;
    }
  }
}

#ifdef USE_BOOST_UNION_FOR_POLYGONS

static BoostPolygon CreateRectangle(float x1, float y1,
                                    float x2, float y2)
{
  BoostPolygon r;
  boost::geometry::append(r, BoostPoint(x1, y1));
  boost::geometry::append(r, BoostPoint(x1, y2));
  boost::geometry::append(r, BoostPoint(x2, y2));
  boost::geometry::append(r, BoostPoint(x2, y1));
  return r;
}

#else

namespace OrthancStone
{
  static RtStructRectangleInSlab CreateRectangle(float x1, float y1,
    float x2, float y2)
  {
    RtStructRectangleInSlab rect;
    rect.xmin = std::min(x1, x2);
    rect.xmax = std::max(x1, x2);
    rect.ymin = std::min(y1, y2);
    rect.ymax = std::max(y1, y2);
    return rect;
  }

  bool CompareRectanglesForProjection(const std::pair<RtStructRectangleInSlab,double>& r1, const std::pair<RtStructRectangleInSlab, double>& r2)
  {
    return r1.second < r2.second;
  }

  bool CompareSlabsY(const RtStructRectanglesInSlab& r1, const RtStructRectanglesInSlab& r2)
  {
    if ((r1.size() == 0) || (r2.size() == 0))
      return false;

    return r1[0].ymax < r2[0].ymax;
  }
}

#endif

namespace OrthancStone
{
  static const OrthancPlugins::DicomTag DICOM_TAG_CONTOUR_GEOMETRIC_TYPE(0x3006, 0x0042);
  static const OrthancPlugins::DicomTag DICOM_TAG_CONTOUR_IMAGE_SEQUENCE(0x3006, 0x0016);
  static const OrthancPlugins::DicomTag DICOM_TAG_CONTOUR_SEQUENCE(0x3006, 0x0040);
  static const OrthancPlugins::DicomTag DICOM_TAG_CONTOUR_DATA(0x3006, 0x0050);
  static const OrthancPlugins::DicomTag DICOM_TAG_NUMBER_OF_CONTOUR_POINTS(0x3006, 0x0046);
  static const OrthancPlugins::DicomTag DICOM_TAG_REFERENCED_SOP_INSTANCE_UID(0x0008, 0x1155);
  static const OrthancPlugins::DicomTag DICOM_TAG_ROI_CONTOUR_SEQUENCE(0x3006, 0x0039);
  static const OrthancPlugins::DicomTag DICOM_TAG_ROI_DISPLAY_COLOR(0x3006, 0x002a);
  static const OrthancPlugins::DicomTag DICOM_TAG_ROI_NAME(0x3006, 0x0026);
  static const OrthancPlugins::DicomTag DICOM_TAG_RT_ROI_INTERPRETED_TYPE(0x3006, 0x00a4);
  static const OrthancPlugins::DicomTag DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE(0x3006, 0x0080);
  static const OrthancPlugins::DicomTag DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE(0x3006, 0x0020);


  static uint8_t ConvertColor(double v)
  {
    if (v < 0)
    {
      return 0;
    }
    else if (v >= 255)
    {
      return 255;
    }
    else
    {
      return static_cast<uint8_t>(v);
    }
  }


  static bool ParseVector(Vector& target,
                          const OrthancPlugins::IDicomDataset& dataset,
                          const OrthancPlugins::DicomPath& tag)
  {
    std::string value;
    return (dataset.GetStringValue(value, tag) &&
            LinearAlgebra::ParseVector(target, value));
  }

  void DicomStructureSet::Polygon::CheckPointIsOnSlice(const Vector& v) const
  {
    if (hasSlice_)
    {
      double magnitude =
        GeometryToolbox::ProjectAlongNormal(v, geometry_.GetNormal());
      if(!LinearAlgebra::IsNear(
        magnitude,
        projectionAlongNormal_,
        sliceThickness_ / 2.0 /* in mm */ ))
      {
        LOG(ERROR) << "This RT-STRUCT contains a point that is off the "
          << "slice of its instance | "
          << "magnitude = " << magnitude << " | "
          << "projectionAlongNormal_ = " << projectionAlongNormal_ << " | "
          << "tolerance (sliceThickness_ / 2.0) = " << (sliceThickness_ / 2.0);

        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
  }

  bool DicomStructureSet::Polygon::IsPointOnSliceIfAny(const Vector& v) const
  {
    if (hasSlice_)
    {
      double magnitude = 
        GeometryToolbox::ProjectAlongNormal(v, geometry_.GetNormal());
      bool onSlice = LinearAlgebra::IsNear(
        magnitude,
        projectionAlongNormal_,
        sliceThickness_ / 2.0 /* in mm */);
      if (!onSlice)
      {
        LOG(WARNING) << "This RT-STRUCT contains a point that is off the "
          << "slice of its instance | "
          << "magnitude = " << magnitude << " | "
          << "projectionAlongNormal_ = " << projectionAlongNormal_ << " | "
          << "tolerance (sliceThickness_ / 2.0) = " << (sliceThickness_ / 2.0);
      }
      return onSlice;
    }
    else
    {
      return true;
    }
  }

  void DicomStructureSet::Polygon::AddPoint(const Vector& v)
  {
#if 1
    // BGO 2019-09-03
    if (IsPointOnSliceIfAny(v))
    {
      points_.push_back(v);
    }
#else
    CheckPoint(v);
    points_.push_back(v);
#endif 
  }


  bool DicomStructureSet::Polygon::UpdateReferencedSlice(const ReferencedSlices& slices)
  {
    if (hasSlice_)
    {
      return true;
    }
    else
    {
      ReferencedSlices::const_iterator it = slices.find(sopInstanceUid_);
      
      if (it == slices.end())
      {
        return false;
      }
      else
      {
        const CoordinateSystem3D& geometry = it->second.geometry_;
        
        hasSlice_ = true;
        geometry_ = geometry;
        projectionAlongNormal_ = GeometryToolbox::ProjectAlongNormal(geometry.GetOrigin(), geometry.GetNormal());
        sliceThickness_ = it->second.thickness_;

        extent_.Reset();
        
        for (Points::const_iterator it = points_.begin(); it != points_.end(); ++it)
        {
          if (IsPointOnSliceIfAny(*it))
          {
            double x, y;
            geometry.ProjectPoint(x, y, *it);
            extent_.AddPoint(x, y);
          }
        }
        return true;
      }
    }
  }

  bool DicomStructureSet::Polygon::IsOnSlice(const CoordinateSystem3D& slice) const
  {
    bool isOpposite = false;

    if (points_.empty() ||
        !hasSlice_ ||
        !GeometryToolbox::IsParallelOrOpposite(isOpposite, slice.GetNormal(), geometry_.GetNormal()))
    {
      return false;
    }
    
    double d = GeometryToolbox::ProjectAlongNormal(slice.GetOrigin(), geometry_.GetNormal());

    return (LinearAlgebra::IsNear(d, projectionAlongNormal_,
                                  sliceThickness_ / 2.0));
  }
    
  bool DicomStructureSet::Polygon::Project(double& x1,
                                           double& y1,
                                           double& x2,
                                           double& y2,
                                           const CoordinateSystem3D& slice) const
  {
    // TODO: optimize this method using a sweep-line algorithm for polygons
    
    if (!hasSlice_ ||
        points_.size() <= 1)
    {
      return false;
    }

    double x, y;
    geometry_.ProjectPoint(x, y, slice.GetOrigin());
      
    bool isOpposite;
    if (GeometryToolbox::IsParallelOrOpposite
        (isOpposite, slice.GetNormal(), geometry_.GetAxisY()))
    {
      // plane is constant Y

      if (y < extent_.GetY1() ||
          y > extent_.GetY2())
      {
        // The polygon does not intersect the input slice
        return false;
      }
        
      bool isFirst = true;
      double xmin = std::numeric_limits<double>::infinity();
      double xmax = -std::numeric_limits<double>::infinity();

      double prevX, prevY;
      geometry_.ProjectPoint(prevX, prevY, points_[points_.size() - 1]);
        
      for (size_t i = 0; i < points_.size(); i++)
      {
        // Reference: ../../Resources/Computations/IntersectSegmentAndHorizontalLine.py
        double curX, curY;
        geometry_.ProjectPoint(curX, curY, points_[i]);

        // if prev* and cur* are on opposite sides of y, this means that the
        // segment intersects the plane.
        if ((prevY < y && curY > y) ||
            (prevY > y && curY < y))
        {
          double p = (curX * prevY - curY * prevX + y * (prevX - curX)) / (prevY - curY);
          xmin = std::min(xmin, p);
          xmax = std::max(xmax, p);
          isFirst = false;

          // xmin and xmax represent the extent of the rectangle along the 
          // intersection between the plane and the polygon geometry

        }

        prevX = curX;
        prevY = curY;
      }
        
      // if NO segment intersects the plane
      if (isFirst)
      {
        return false;
      }
      else
      {
        // y is the plane y coord in the polygon geometry
        // xmin and xmax are ALSO expressed in the polygon geometry

        // let's convert them to 3D world geometry...
        Vector p1 = (geometry_.MapSliceToWorldCoordinates(xmin, y) +
                     sliceThickness_ / 2.0 * geometry_.GetNormal());
        Vector p2 = (geometry_.MapSliceToWorldCoordinates(xmax, y) -
                     sliceThickness_ / 2.0 * geometry_.GetNormal());
          
        // then to the cutting plane geometry...
        slice.ProjectPoint(x1, y1, p1);
        slice.ProjectPoint(x2, y2, p2);
        return true;
      }
    }
    else if (GeometryToolbox::IsParallelOrOpposite
             (isOpposite, slice.GetNormal(), geometry_.GetAxisX()))
    {
      // plane is constant X

      /*
      Please read the comments in the section above, by taking into account
      the fact that, in this case, the plane has a constant X, not Y (in 
      polygon geometry_ coordinates)
      */

      if (x < extent_.GetX1() ||
          x > extent_.GetX2())
      {
        return false;
      }

      bool isFirst = true;
      double ymin = std::numeric_limits<double>::infinity();
      double ymax = -std::numeric_limits<double>::infinity();

      double prevX, prevY;
      geometry_.ProjectPoint(prevX, prevY, points_[points_.size() - 1]);
        
      for (size_t i = 0; i < points_.size(); i++)
      {
        // Reference: ../../Resources/Computations/IntersectSegmentAndVerticalLine.py
        double curX, curY;
        geometry_.ProjectPoint(curX, curY, points_[i]);

        if ((prevX < x && curX > x) ||
            (prevX > x && curX < x))
        {
          double p = (curX * prevY - curY * prevX + x * (curY - prevY)) / (curX - prevX);
          ymin = std::min(ymin, p);
          ymax = std::max(ymax, p);
          isFirst = false;
        }

        prevX = curX;
        prevY = curY;
      }
        
      if (isFirst)
      {
        return false;
      }
      else
      {
        Vector p1 = (geometry_.MapSliceToWorldCoordinates(x, ymin) +
                     sliceThickness_ / 2.0 * geometry_.GetNormal());
        Vector p2 = (geometry_.MapSliceToWorldCoordinates(x, ymax) -
                     sliceThickness_ / 2.0 * geometry_.GetNormal());

        slice.ProjectPoint(x1, y1, p1);
        slice.ProjectPoint(x2, y2, p2);

        // TODO WHY THIS???
        y1 = -y1;
        y2 = -y2;

        return true;
      }
    }
    else
    {
      // Should not happen
      return false;
    }
  }

  
  const DicomStructureSet::Structure& DicomStructureSet::GetStructure(size_t index) const
  {
    if (index >= structures_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    return structures_[index];
  }


  DicomStructureSet::Structure& DicomStructureSet::GetStructure(size_t index)
  {
    if (index >= structures_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    return structures_[index];
  }

  DicomStructureSet::DicomStructureSet(const OrthancPlugins::FullOrthancDataset& tags)
  {
    OrthancPlugins::DicomDatasetReader reader(tags);
    
    size_t count, tmp;
    if (!tags.GetSequenceSize(count, DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE) ||
        !tags.GetSequenceSize(tmp, DICOM_TAG_ROI_CONTOUR_SEQUENCE) ||
        tmp != count ||
        !tags.GetSequenceSize(tmp, DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE) ||
        tmp != count)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    structures_.resize(count);
    for (size_t i = 0; i < count; i++)
    {
      structures_[i].interpretation_ = reader.GetStringValue
        (OrthancPlugins::DicomPath(DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE, i,
                                   DICOM_TAG_RT_ROI_INTERPRETED_TYPE),
         "No interpretation");

      structures_[i].name_ = reader.GetStringValue
        (OrthancPlugins::DicomPath(DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE, i,
                                   DICOM_TAG_ROI_NAME),
         "No name");

      Vector color;
      if (ParseVector(color, tags, OrthancPlugins::DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                             DICOM_TAG_ROI_DISPLAY_COLOR)) &&
          color.size() == 3)
      {
        structures_[i].red_ = ConvertColor(color[0]);
        structures_[i].green_ = ConvertColor(color[1]);
        structures_[i].blue_ = ConvertColor(color[2]);
      }
      else
      {
        structures_[i].red_ = 255;
        structures_[i].green_ = 0;
        structures_[i].blue_ = 0;
      }

      size_t countSlices;
      if (!tags.GetSequenceSize(countSlices, OrthancPlugins::DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                                       DICOM_TAG_CONTOUR_SEQUENCE)))
      {
        countSlices = 0;
      }

      LOG(INFO) << "New RT structure: \"" << structures_[i].name_ 
                   << "\" with interpretation \"" << structures_[i].interpretation_
                   << "\" containing " << countSlices << " slices (color: " 
                   << static_cast<int>(structures_[i].red_) << "," 
                   << static_cast<int>(structures_[i].green_) << ","
                   << static_cast<int>(structures_[i].blue_) << ")";

      // These temporary variables avoid allocating many vectors in the loop below
      OrthancPlugins::DicomPath countPointsPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                                DICOM_TAG_NUMBER_OF_CONTOUR_POINTS);

      OrthancPlugins::DicomPath geometricTypePath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                  DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                                  DICOM_TAG_CONTOUR_GEOMETRIC_TYPE);
      
      OrthancPlugins::DicomPath imageSequencePath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                  DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                                  DICOM_TAG_CONTOUR_IMAGE_SEQUENCE);

      // (3006,0039)[i] / (0x3006, 0x0040)[0] / (0x3006, 0x0016)[0] / (0x0008, 0x1155)
      OrthancPlugins::DicomPath referencedInstancePath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                       DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                                       DICOM_TAG_CONTOUR_IMAGE_SEQUENCE, 0,
                                                       DICOM_TAG_REFERENCED_SOP_INSTANCE_UID);

      OrthancPlugins::DicomPath contourDataPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                                DICOM_TAG_CONTOUR_DATA);

      for (size_t j = 0; j < countSlices; j++)
      {
        unsigned int countPoints;

        countPointsPath.SetPrefixIndex(1, j);
        if (!reader.GetUnsignedIntegerValue(countPoints, countPointsPath))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
            
        //LOG(INFO) << "Parsing slice containing " << countPoints << " vertices";

        geometricTypePath.SetPrefixIndex(1, j);
        std::string type = reader.GetMandatoryStringValue(geometricTypePath);
        if (type != "CLOSED_PLANAR")
        {
          LOG(WARNING) << "Ignoring contour with geometry type: " << type;
          continue;
        }

        size_t size;

        imageSequencePath.SetPrefixIndex(1, j);
        if (!tags.GetSequenceSize(size, imageSequencePath) || size != 1)
        {
          LOG(ERROR) << "The ContourImageSequence sequence (tag 3006,0016) must be present and contain one entry.";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
        }

        referencedInstancePath.SetPrefixIndex(1, j);
        std::string sopInstanceUid = reader.GetMandatoryStringValue(referencedInstancePath);

        contourDataPath.SetPrefixIndex(1, j);        
        std::string slicesData = reader.GetMandatoryStringValue(contourDataPath);

        Vector points;
        if (!LinearAlgebra::ParseVector(points, slicesData) ||
            points.size() != 3 * countPoints)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
        }

        // seen in real world
        if(Orthanc::Toolbox::StripSpaces(sopInstanceUid) == "") 
        {
          LOG(ERROR) << "WARNING. The following Dicom tag (Referenced SOP Instance UID) contains an empty value : // (3006,0039)[" << i << "] / (0x3006, 0x0040)[0] / (0x3006, 0x0016)[0] / (0x0008, 0x1155)";
        }

        Polygon polygon(sopInstanceUid);
        polygon.Reserve(countPoints);

        for (size_t k = 0; k < countPoints; k++)
        {
          Vector v(3);
          v[0] = points[3 * k];
          v[1] = points[3 * k + 1];
          v[2] = points[3 * k + 2];
          polygon.AddPoint(v);
        }

        structures_[i].polygons_.push_back(polygon);
      }
    }
  }


  Vector DicomStructureSet::GetStructureCenter(size_t index) const
  {
    const Structure& structure = GetStructure(index);

    Vector center;
    LinearAlgebra::AssignVector(center, 0, 0, 0);
    if (structure.polygons_.empty())
    {
      return center;
    }

    double n = static_cast<double>(structure.polygons_.size());

    for (Polygons::const_iterator polygon = structure.polygons_.begin();
         polygon != structure.polygons_.end(); ++polygon)
    {
      if (!polygon->GetPoints().empty())
      {
        center += polygon->GetPoints().front() / n;
      }
    }

    return center;
  }


  const std::string& DicomStructureSet::GetStructureName(size_t index) const
  {
    return GetStructure(index).name_;
  }


  const std::string& DicomStructureSet::GetStructureInterpretation(size_t index) const
  {
    return GetStructure(index).interpretation_;
  }


  Color DicomStructureSet::GetStructureColor(size_t index) const
  {
    const Structure& s = GetStructure(index);
    return Color(s.red_, s.green_, s.blue_);
  }
  
    
  void DicomStructureSet::GetStructureColor(uint8_t& red,
                                            uint8_t& green,
                                            uint8_t& blue,
                                            size_t index) const
  {
    const Structure& s = GetStructure(index);
    red = s.red_;
    green = s.green_;
    blue = s.blue_;
  }


  void DicomStructureSet::GetReferencedInstances(std::set<std::string>& instances)
  {
    for (Structures::const_iterator structure = structures_.begin();
         structure != structures_.end(); ++structure)
    {
      for (Polygons::const_iterator polygon = structure->polygons_.begin();
           polygon != structure->polygons_.end(); ++polygon)
      {
        instances.insert(polygon->GetSopInstanceUid());
      }
    }
  }


  void DicomStructureSet::AddReferencedSlice(const std::string& sopInstanceUid,
                                             const std::string& seriesInstanceUid,
                                             const CoordinateSystem3D& geometry,
                                             double thickness)
  {
    if (referencedSlices_.find(sopInstanceUid) != referencedSlices_.end())
    {
      // This geometry is already known
      LOG(ERROR) << "DicomStructureSet::AddReferencedSlice(): (referencedSlices_.find(sopInstanceUid) != referencedSlices_.end()). sopInstanceUid = " << sopInstanceUid;
     
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      if (thickness < 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
        
      if (!referencedSlices_.empty())
      {
        const ReferencedSlice& reference = referencedSlices_.begin()->second;

        if (reference.seriesInstanceUid_ != seriesInstanceUid)
        {
          LOG(ERROR) << "This RT-STRUCT refers to several different series";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }

        if (!GeometryToolbox::IsParallel(reference.geometry_.GetNormal(), geometry.GetNormal()))
        {
          LOG(ERROR) << "The slices in this RT-STRUCT are not parallel";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
      }
        
      referencedSlices_[sopInstanceUid] = ReferencedSlice(seriesInstanceUid, geometry, thickness);

      for (Structures::iterator structure = structures_.begin();
           structure != structures_.end(); ++structure)
      {
        for (Polygons::iterator polygon = structure->polygons_.begin();
             polygon != structure->polygons_.end(); ++polygon)
        {
          polygon->UpdateReferencedSlice(referencedSlices_);
        }
      }
    }
  }


  void DicomStructureSet::AddReferencedSlice(const Orthanc::DicomMap& dataset)
  {
    CoordinateSystem3D slice(dataset);

    double thickness = 1;  // 1 mm by default

    std::string s;
    Vector v;
    if (dataset.LookupStringValue(s, Orthanc::DICOM_TAG_SLICE_THICKNESS, false) &&
        LinearAlgebra::ParseVector(v, s) &&
        v.size() > 0)
    {
      thickness = v[0];
    }

    std::string instance, series;
    if (dataset.LookupStringValue(instance, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false) &&
        dataset.LookupStringValue(series, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false))
    {
      AddReferencedSlice(instance, series, slice, thickness);
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }


  void DicomStructureSet::CheckReferencedSlices()
  {
    for (Structures::iterator structure = structures_.begin();
         structure != structures_.end(); ++structure)
    {
      for (Polygons::iterator polygon = structure->polygons_.begin();
           polygon != structure->polygons_.end(); ++polygon)
      {
        if (!polygon->UpdateReferencedSlice(referencedSlices_))
        {
          std::string sopInstanceUid = polygon->GetSopInstanceUid();
          if (Orthanc::Toolbox::StripSpaces(sopInstanceUid) == "")
          {
            LOG(ERROR) << "DicomStructureSet::CheckReferencedSlices(): "
              << " missing information about referenced instance "
              << "(sopInstanceUid is empty!)";
          }
          else
          {
            LOG(ERROR) << "DicomStructureSet::CheckReferencedSlices(): "
              << " missing information about referenced instance "
              << "(sopInstanceUid = " << sopInstanceUid << ")";
          }
          //throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }
      }
    }
  }


  Vector DicomStructureSet::GetNormal() const
  {
    if (referencedSlices_.empty())
    {
      Vector v;
      LinearAlgebra::AssignVector(v, 0, 0, 1);
      return v;
    }
    else
    {
      return referencedSlices_.begin()->second.geometry_.GetNormal();
    }
  }

#ifdef USE_BOOST_UNION_FOR_POLYGONS 
  bool DicomStructureSet::ProjectStructure(std::vector< std::vector<Point2D> >& polygons,
                                           const Structure& structure,
                                           const CoordinateSystem3D& slice) const
#else
  bool DicomStructureSet::ProjectStructure(std::vector< std::pair<Point2D, Point2D> >& segments,
    const Structure& structure,
    const CoordinateSystem3D& slice) const
#endif
  {
#ifdef USE_BOOST_UNION_FOR_POLYGONS 
    polygons.clear();
#else
    segments.clear();
#endif

    Vector normal = GetNormal();
    
    bool isOpposite;    
    if (GeometryToolbox::IsParallelOrOpposite(isOpposite, normal, slice.GetNormal()))
    {
      // This is an axial projection

      for (Polygons::const_iterator polygon = structure.polygons_.begin();
           polygon != structure.polygons_.end(); ++polygon)
      {
        if (polygon->IsOnSlice(slice))
        {
#ifdef USE_BOOST_UNION_FOR_POLYGONS 
          polygons.push_back(std::vector<Point2D>());
          
          for (Points::const_iterator p = polygon->GetPoints().begin();
               p != polygon->GetPoints().end(); ++p)
          {
            double x, y;
            slice.ProjectPoint(x, y, *p);
            polygons.back().push_back(Point2D(x, y));
          }
#else
          // we need to add all the segments corresponding to this polygon
          const std::vector<Vector>& points3D = polygon->GetPoints();
          if (points3D.size() >= 3)
          {
            Point2D prev2D;
            {
              Vector prev = points3D[0];
              double prevX, prevY;
              slice.ProjectPoint(prevX, prevY, prev);
              prev2D = Point2D(prevX, prevY);
            }

            size_t pointCount = points3D.size();
            for (size_t ipt = 1; ipt < pointCount; ++ipt)
            {
              Vector next = points3D[ipt];
              double nextX, nextY;
              slice.ProjectPoint(nextX, nextY, next);
              Point2D next2D(nextX, nextY);
              segments.push_back(std::pair<Point2D, Point2D>(prev2D, next2D));
              prev2D = next2D;
            }
          }
          else
          {
            LOG(ERROR) << "Contour with less than 3 points!";
            // !!!
          }
#endif
        }
      }

      return true;
    }
    else if (GeometryToolbox::IsParallelOrOpposite(isOpposite, normal, slice.GetAxisX()) ||
             GeometryToolbox::IsParallelOrOpposite(isOpposite, normal, slice.GetAxisY()))
    {
#if 1
      // Sagittal or coronal projection

#ifdef USE_BOOST_UNION_FOR_POLYGONS 
      std::vector<BoostPolygon> projected;
#else
      // this will contain the intersection of the polygon slab with
      // the cutting plane, projected on the cutting plane coord system 
      // (that yields a rectangle) + the Z coordinate of the polygon 
      // (this is required to group polygons with the same Z later)
      std::vector<std::pair<RtStructRectangleInSlab, double> > projected;
#endif
      for (Polygons::const_iterator polygon = structure.polygons_.begin();
           polygon != structure.polygons_.end(); ++polygon)
      {
        double x1, y1, x2, y2;

        if (polygon->Project(x1, y1, x2, y2, slice))
        {
          double curZ = polygon->GetGeometryOrigin()[2];

          // x1,y1 and x2,y2 are in "slice" coordinates (the cutting plane 
          // geometry)
          projected.push_back(std::make_pair(CreateRectangle(
            static_cast<float>(x1), 
            static_cast<float>(y1), 
            static_cast<float>(x2), 
            static_cast<float>(y2)),curZ));
        }
      }
#ifndef USE_BOOST_UNION_FOR_POLYGONS
      // projected contains a set of rectangles specified by two opposite
      // corners (x1,y1,x2,y2)
      // we need to merge them 
      // each slab yields ONE polygon!

      // we need to sorted all the rectangles that originate from the same Z
      // into lanes. To make sure they are grouped together in the array, we
      // sort it.
      std::sort(projected.begin(), projected.end(), CompareRectanglesForProjection);

      std::vector<RtStructRectanglesInSlab> rectanglesForEachSlab;
      rectanglesForEachSlab.reserve(projected.size());

      double curZ = 0;
      for (size_t i = 0; i < projected.size(); ++i)
      {
#if 0
        rectanglesForEachSlab.push_back(RtStructRectanglesInSlab());
#else
        if (i == 0)
        {
          curZ = projected[i].second;
          rectanglesForEachSlab.push_back(RtStructRectanglesInSlab());
        }
        else
        {
          // this check is needed to prevent creating a new slab if 
          // the new polygon is at the same Z coord than last one
          if (!LinearAlgebra::IsNear(curZ, projected[i].second))
          {
            rectanglesForEachSlab.push_back(RtStructRectanglesInSlab());
            curZ = projected[i].second;
          }
        }
#endif

        rectanglesForEachSlab.back().push_back(projected[i].first);

        // as long as they have the same y, we should put them into the same lane
        // BUT in Sebastien's code, there is only one polygon per lane.

        //std::cout << "rect: xmin = " << rect.xmin << " xmax = " << rect.xmax << " ymin = " << rect.ymin << " ymax = " << rect.ymax << std::endl;
      }
      
      // now we need to sort the slabs in increasing Y order (see ConvertListOfSlabsToSegments)
      std::sort(rectanglesForEachSlab.begin(), rectanglesForEachSlab.end(), CompareSlabsY);

      ConvertListOfSlabsToSegments(segments, rectanglesForEachSlab, projected.size());
#else
      BoostMultiPolygon merged;
      Union(merged, projected);

      polygons.resize(merged.size());
      for (size_t i = 0; i < merged.size(); i++)
      {
        const std::vector<BoostPoint>& outer = merged[i].outer();

        polygons[i].resize(outer.size());
        for (size_t j = 0; j < outer.size(); j++)
        {
          polygons[i][j] = Point2D(outer[j].x(), outer[j].y());
        }
      }  
#endif

#else
      for (Polygons::iterator polygon = structure.polygons_.begin();
           polygon != structure.polygons_.end(); ++polygon)
      {
        double x1, y1, x2, y2;
        if (polygon->Project(x1, y1, x2, y2, slice))
        {
          std::vector<Point2D> p(4);
          p[0] = std::make_pair(x1, y1);
          p[1] = std::make_pair(x2, y1);
          p[2] = std::make_pair(x2, y2);
          p[3] = std::make_pair(x1, y2);
          polygons.push_back(p);
        }
      }
#endif
      
      return true;
    }
    else
    {
      return false;
    }
  }
}
