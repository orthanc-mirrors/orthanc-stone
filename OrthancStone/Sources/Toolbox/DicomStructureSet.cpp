/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 **/


#define USE_BOOST_UNION_FOR_POLYGONS 0

#include "DicomStructureSet.h"

#include "BucketAccumulator2D.h"
#include "GenericToolbox.h"
#include "GeometryToolbox.h"
#include "OrthancDatasets/DicomDatasetReader.h"

#include <Logging.h>
#include <OrthancException.h>
#include <Toolbox.h>

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4244)
#endif

#if STONE_TIME_BLOCKING_OPS
#  include <boost/date_time/posix_time/posix_time.hpp>
#endif

#if !defined(USE_BOOST_UNION_FOR_POLYGONS)
#  error Macro USE_BOOST_UNION_FOR_POLYGONS must be defined
#endif

#include <limits>
#include <stdio.h>
#include <boost/math/constants/constants.hpp>

#if USE_BOOST_UNION_FOR_POLYGONS == 1
#  include <boost/geometry.hpp>
#  include <boost/geometry/geometries/point_xy.hpp>
#  include <boost/geometry/geometries/polygon.hpp>
#  include <boost/geometry/multi/geometries/multi_polygon.hpp>
#else
#  include "DicomStructureSetUtils.h"  // TODO REMOVE
#  include "UnionOfRectangles.h"
#endif

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#if ORTHANC_ENABLE_DCMTK == 1
#  include "ParsedDicomDataset.h"
#endif


#if USE_BOOST_UNION_FOR_POLYGONS == 1

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

  bool CompareRectanglesForProjection(const std::pair<RtStructRectangleInSlab,double>& r1,
                                      const std::pair<RtStructRectangleInSlab, double>& r2)
  {
    return r1.second < r2.second;
  }

  bool CompareSlabsY(const RtStructRectanglesInSlab& r1,
                     const RtStructRectanglesInSlab& r2)
  {
    if ((r1.size() == 0) || (r2.size() == 0))
      return false;

    return r1[0].ymax < r2[0].ymax;
  }
}

#endif

namespace OrthancStone
{
  static const Orthanc::DicomTag DICOM_TAG_CONTOUR_GEOMETRIC_TYPE(0x3006, 0x0042);
  static const Orthanc::DicomTag DICOM_TAG_CONTOUR_IMAGE_SEQUENCE(0x3006, 0x0016);
  static const Orthanc::DicomTag DICOM_TAG_CONTOUR_SEQUENCE(0x3006, 0x0040);
  static const Orthanc::DicomTag DICOM_TAG_CONTOUR_DATA(0x3006, 0x0050);
  static const Orthanc::DicomTag DICOM_TAG_NUMBER_OF_CONTOUR_POINTS(0x3006, 0x0046);
  static const Orthanc::DicomTag DICOM_TAG_REFERENCED_SOP_INSTANCE_UID(0x0008, 0x1155);
  static const Orthanc::DicomTag DICOM_TAG_ROI_CONTOUR_SEQUENCE(0x3006, 0x0039);
  static const Orthanc::DicomTag DICOM_TAG_ROI_DISPLAY_COLOR(0x3006, 0x002a);
  static const Orthanc::DicomTag DICOM_TAG_ROI_NAME(0x3006, 0x0026);
  static const Orthanc::DicomTag DICOM_TAG_RT_ROI_INTERPRETED_TYPE(0x3006, 0x00a4);
  static const Orthanc::DicomTag DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE(0x3006, 0x0080);
  static const Orthanc::DicomTag DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE(0x3006, 0x0020);


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


  static bool FastParseVector(Vector& target,
                              const IDicomDataset& dataset,
                              const Orthanc::DicomPath& tag)
  {
    std::string value;
    return (dataset.GetStringValue(value, tag) &&
            GenericToolbox::FastParseVector(target, value));
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
        // return true;  // TODO - TEST
        
        const CoordinateSystem3D& geometry = it->second.geometry_;
        
        hasSlice_ = true;
        geometry_ = geometry;
        projectionAlongNormal_ = GeometryToolbox::ProjectAlongNormal(geometry.GetOrigin(), geometry.GetNormal());
        sliceThickness_ = it->second.thickness_;

        extent_.Clear();
        
        for (Points::const_iterator it2 = points_.begin(); it2 != points_.end(); ++it2)
        {
          if (IsPointOnSliceIfAny(*it2))
          {
            double x, y;
            geometry.ProjectPoint2(x, y, *it2);
            extent_.AddPoint(x, y);
          }
        }
        return true;
      }
    }
  }

  bool DicomStructureSet::Polygon::IsOnSlice(const CoordinateSystem3D& slice,
                                             const Vector& estimatedNormal,
                                             double estimatedSliceThickness) const
  {
    bool isOpposite = false;

    if (points_.empty())
    {
      return false;
    }
    else if (hasSlice_)
    {
      // Use the actual geometry of this specific slice
      if (!GeometryToolbox::IsParallelOrOpposite(isOpposite, slice.GetNormal(), geometry_.GetNormal()))
      {
        return false;
      }
      else
      {
        double d = GeometryToolbox::ProjectAlongNormal(slice.GetOrigin(), geometry_.GetNormal());
        return (LinearAlgebra::IsNear(d, projectionAlongNormal_, sliceThickness_ / 2.0));
      }
    }
    else
    {
      // Use the estimated geometry for the global RT-STRUCT volume
      if (!GeometryToolbox::IsParallelOrOpposite(isOpposite, slice.GetNormal(), estimatedNormal))
      {
        return false;
      }
      else
      {
        double d1 = GeometryToolbox::ProjectAlongNormal(slice.GetOrigin(), estimatedNormal);
        double d2 = GeometryToolbox::ProjectAlongNormal(points_.front(), estimatedNormal);
        return (LinearAlgebra::IsNear(d1, d2, estimatedSliceThickness / 2.0));
      }
    }
  }
    
  bool DicomStructureSet::Polygon::Project(double& x1,
                                           double& y1,
                                           double& x2,
                                           double& y2,
                                           const CoordinateSystem3D& slice,
                                           const Vector& estimatedNormal,
                                           double estimatedSliceThickness) const
  {
    if (!hasSlice_ ||
        points_.size() <= 1)
    {
      return false;
    }

    double x, y;
    geometry_.ProjectPoint2(x, y, slice.GetOrigin());
      
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
      geometry_.ProjectPoint2(prevX, prevY, points_[points_.size() - 1]);
        
      for (size_t i = 0; i < points_.size(); i++)
      {
        // Reference: ../../Resources/Computations/IntersectSegmentAndHorizontalLine.py
        double curX, curY;
        geometry_.ProjectPoint2(curX, curY, points_[i]);

        // if prev* and cur* are on opposite sides of y, this means that the
        // segment intersects the plane.
        if ((prevY <= y && curY >= y) ||
            (prevY >= y && curY <= y))
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
        slice.ProjectPoint2(x1, y1, p1);
        slice.ProjectPoint2(x2, y2, p2);
        return true;
      }
    }
    else if (GeometryToolbox::IsParallelOrOpposite
             (isOpposite, slice.GetNormal(), geometry_.GetAxisX()))
    {
      // plane is constant X => Sagittal view (remember that in the
      // sagittal projection, the normal must be swapped)

      
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
      geometry_.ProjectPoint2(prevX, prevY, points_[points_.size() - 1]);
        
      for (size_t i = 0; i < points_.size(); i++)
      {
        // Reference: ../../Resources/Computations/IntersectSegmentAndVerticalLine.py
        double curX, curY;
        geometry_.ProjectPoint2(curX, curY, points_[i]);

        if ((prevX <= x && curX >= x) ||
            (prevX >= x && curX <= x))
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

        slice.ProjectPoint2(x1, y1, p1);
        slice.ProjectPoint2(x2, y2, p2);

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

  void DicomStructureSet::Setup(const IDicomDataset& tags)
  {
#if STONE_TIME_BLOCKING_OPS
    boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::universal_time();
#endif

    DicomDatasetReader reader(tags);
    
    size_t count, tmp;
    if (!tags.GetSequenceSize(count, Orthanc::DicomPath(DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE)) ||
        !tags.GetSequenceSize(tmp, Orthanc::DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE)) ||
        tmp != count ||
        !tags.GetSequenceSize(tmp, Orthanc::DicomPath(DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE)) ||
        tmp != count)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    structures_.resize(count);
    for (size_t i = 0; i < count; i++)
    {
      structures_[i].interpretation_ = reader.GetStringValue
        (Orthanc::DicomPath(DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE, i,
                            DICOM_TAG_RT_ROI_INTERPRETED_TYPE),
         "No interpretation");

      structures_[i].name_ = reader.GetStringValue
        (Orthanc::DicomPath(DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE, i,
                            DICOM_TAG_ROI_NAME),
         "No name");

      Vector color;
      if (FastParseVector(color, tags, Orthanc::DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
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
      if (!tags.GetSequenceSize(countSlices, Orthanc::DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
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

      /**
       * These temporary variables avoid allocating many vectors in
       * the loop below (indeed, "Orthanc::DicomPath" handles a
       * "std::vector<PrefixItem>")
       **/
      Orthanc::DicomPath countPointsPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                         DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                         DICOM_TAG_NUMBER_OF_CONTOUR_POINTS);

      Orthanc::DicomPath geometricTypePath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                           DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                           DICOM_TAG_CONTOUR_GEOMETRIC_TYPE);
      
      Orthanc::DicomPath imageSequencePath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                           DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                           DICOM_TAG_CONTOUR_IMAGE_SEQUENCE);

      // (3006,0039)[i] / (0x3006, 0x0040)[0] / (0x3006, 0x0016)[0] / (0x0008, 0x1155)
      Orthanc::DicomPath referencedInstancePath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                DICOM_TAG_CONTOUR_SEQUENCE, 0,
                                                DICOM_TAG_CONTOUR_IMAGE_SEQUENCE, 0,
                                                DICOM_TAG_REFERENCED_SOP_INSTANCE_UID);

      Orthanc::DicomPath contourDataPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
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

        if (!GenericToolbox::FastParseVector(points, slicesData) ||
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

    EstimateGeometry();
    
#if STONE_TIME_BLOCKING_OPS
    boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration duration = timerEnd - timerStart;
    int64_t durationMs = duration.total_milliseconds();
    LOG(WARNING) << "DicomStructureSet::Setup took " << durationMs << " ms";
#endif
  }


#if ORTHANC_ENABLE_DCMTK == 1
  DicomStructureSet::DicomStructureSet(Orthanc::ParsedDicomFile& instance)
  {
    ParsedDicomDataset dataset(instance);
    Setup(dataset);
  }
#endif
  

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
  
    
  void DicomStructureSet::GetReferencedInstances(std::set<std::string>& instances) const
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

  bool DicomStructureSet::ProjectStructure(std::vector< std::vector<ScenePoint2D> >& chains,
                                           const Structure& structure,
                                           const CoordinateSystem3D& sourceSlice) const
  {
    const CoordinateSystem3D slice = CoordinateSystem3D::NormalizeCuttingPlane(sourceSlice);
    
    chains.clear();

    Vector normal = GetNormal();
    
    bool isOpposite;    
    if (GeometryToolbox::IsParallelOrOpposite(isOpposite, normal, slice.GetNormal()))
    {
      // This is an axial projection

      chains.reserve(structure.polygons_.size());
      
      for (Polygons::const_iterator polygon = structure.polygons_.begin();
           polygon != structure.polygons_.end(); ++polygon)
      {
        const Points& points = polygon->GetPoints();
        
        if (polygon->IsOnSlice(slice, GetEstimatedNormal(), GetEstimatedSliceThickness()) &&
            !points.empty())
        {
          chains.push_back(std::vector<ScenePoint2D>());
          chains.back().reserve(points.size() + 1);
          
          for (Points::const_iterator p = points.begin();
               p != points.end(); ++p)
          {
            double x, y;
            slice.ProjectPoint2(x, y, *p);
            chains.back().push_back(ScenePoint2D(x, y));
          }

          double x0, y0;
          slice.ProjectPoint2(x0, y0, points.front());
          chains.back().push_back(ScenePoint2D(x0, y0));
        }
      }

      return true;
    }
    else if (GeometryToolbox::IsParallelOrOpposite(isOpposite, normal, slice.GetAxisX()) ||
             GeometryToolbox::IsParallelOrOpposite(isOpposite, normal, slice.GetAxisY()))
    {
      // Sagittal or coronal projection

#if USE_BOOST_UNION_FOR_POLYGONS == 1
      std::vector<BoostPolygon> projected;

      for (Polygons::const_iterator polygon = structure.polygons_.begin();
           polygon != structure.polygons_.end(); ++polygon)
      {
        double x1, y1, x2, y2;

        if (polygon->Project(x1, y1, x2, y2, slice))
        {
          projected.push_back(CreateRectangle(x1, y1, x2, y2));
        }
      }

      BoostMultiPolygon merged;
      Union(merged, projected);

      chains.resize(merged.size());
      for (size_t i = 0; i < merged.size(); i++)
      {
        const std::vector<BoostPoint>& outer = merged[i].outer();

        chains[i].resize(outer.size());
        for (size_t j = 0; j < outer.size(); j++)
        {
          chains[i][j] = ScenePoint2D(outer[j].x(), outer[j].y());
        }
      }  

#elif 0

      // TODO - Fix possible infinite loop in UnionOfRectangles
      
      std::list<Extent2D> rectangles;
      
      for (Polygons::const_iterator polygon = structure.polygons_.begin();
           polygon != structure.polygons_.end(); ++polygon)
      {
        double x1, y1, x2, y2;

        if (polygon->Project(x1, y1, x2, y2, slice))
        {
          rectangles.push_back(Extent2D(x1, y1, x2, y2));
        }
      }

      typedef std::list< std::vector<ScenePoint2D> >  Contours;

      Contours contours;
      UnionOfRectangles::Apply(contours, rectangles);

      chains.reserve(contours.size());
      
      for (Contours::const_iterator it = contours.begin(); it != contours.end(); ++it)
      {
        chains.push_back(*it);
      }
      
#else
      // this will contain the intersection of the polygon slab with
      // the cutting plane, projected on the cutting plane coord system 
      // (that yields a rectangle) + the Z coordinate of the polygon 
      // (this is required to group polygons with the same Z later)
      std::vector<std::pair<RtStructRectangleInSlab, double> > projected;

      for (Polygons::const_iterator polygon = structure.polygons_.begin();
           polygon != structure.polygons_.end(); ++polygon)
      {
        double x1, y1, x2, y2;

        if (polygon->Project(x1, y1, x2, y2, slice, GetEstimatedNormal(), GetEstimatedSliceThickness()))
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

      std::vector< std::pair<ScenePoint2D, ScenePoint2D> > segments;
      ConvertListOfSlabsToSegments(segments, rectanglesForEachSlab, projected.size());

      chains.resize(segments.size());
      for (size_t i = 0; i < segments.size(); i++)
      {
        chains[i].resize(2);
        chains[i][0] = segments[i].first;
        chains[i][1] = segments[i].second;
      }
#endif
      
      return true;
    }
    else
    {
      return false;
    }
  }


  void DicomStructureSet::ProjectOntoLayer(PolylineSceneLayer& layer,
                                           const CoordinateSystem3D& plane,
                                           size_t structureIndex,
                                           const Color& color) const
  {
    std::vector< std::vector<ScenePoint2D> > chains;
    
    if (ProjectStructure(chains, structureIndex, plane))
    {
      for (size_t j = 0; j < chains.size(); j++)
      {
        layer.AddChain(chains[j], false, color.GetRed(), color.GetGreen(), color.GetBlue());
      }
    }
  }


  void DicomStructureSet::GetStructurePoints(std::list< std::vector<Vector> >& target,
                                             size_t structureIndex,
                                             const std::string& sopInstanceUid) const
  {
    target.clear();
    
    const Structure& structure = GetStructure(structureIndex);

    // TODO - Could be optimized by adding a multimap on "Structure", mapping
    // from SOP Instance UID to polygons
    
    for (Polygons::const_iterator it = structure.polygons_.begin();
         it != structure.polygons_.end(); ++it)
    {
      if (it->GetSopInstanceUid() == sopInstanceUid)
      {
        target.push_back(it->GetPoints());
      }
    }
  }


  void DicomStructureSet::EstimateGeometry()
  {
    static const double PI = boost::math::constants::pi<double>();

    BucketAccumulator2D accumulator(0, PI, 9,   /* for acos() */
                                    -PI, PI, 9, /* for atan() */
                                    true /* store values */);

    unsigned int countPolygons = 0;
    for (size_t i = 0; i < structures_.size(); i++)
    {
      const Polygons& polygons = structures_[i].polygons_;

      for (Polygons::const_iterator it = polygons.begin(); it != polygons.end(); ++it)
      {
        countPolygons++;
        
        const Points& points = it->GetPoints();
        
        if (points.size() >= 3)
        {
          // Compute the normal of the polygon using 3 successive points
          Vector normal;
          bool valid = false;

          for (size_t i = 0; i + 2 < points.size(); i++)
          {
            const Vector& a = points[i];
            const Vector& b = points[i + 1];
            const Vector& c = points[i + 2];
            LinearAlgebra::CrossProduct(normal, b - a, c - a);  // (*)
            LinearAlgebra::NormalizeVector(normal);

            if (LinearAlgebra::IsNear(boost::numeric::ublas::norm_2(normal), 1.0))  // (**)
            {
              valid = true;
              break;
            }
          }

          if (valid)
          {
            // Check that all the points of the polygon lie in the plane defined by the normal
            double d1 = GeometryToolbox::ProjectAlongNormal(points[0], normal);
          
            for (size_t i = 1; i < points.size(); i++)
            {
              double d2 = GeometryToolbox::ProjectAlongNormal(points[i], normal);
            
              if (!LinearAlgebra::IsNear(d1, d2))
              {
                valid = false;
                break;
              }
            }
          }

          if (valid)
          {
            if (normal[2] < 0)
            {
              normal = -normal;
            }

            // The normal is a non-zero unit vector because of (*) and (**), so "r == 1"
            // https://en.wikipedia.org/wiki/Vector_fields_in_cylindrical_and_spherical_coordinates#Vector_fields_2
            
            const double theta = acos(normal[2]);
            const double phi = atan(normal[1]);
            accumulator.AddValue(theta, phi);
          }
        }
      }
    }

    size_t bestX, bestY;
    accumulator.FindBestBucket(bestX, bestY);

    if (accumulator.GetBucketContentSize(bestX, bestY) > 0)
    {
      double normalTheta, normalPhi;
      accumulator.ComputeBestMedian(normalTheta, normalPhi);
      
      // Back to (x,y,z) normal, taking "r == 1"
      // https://en.wikipedia.org/wiki/Vector_fields_in_cylindrical_and_spherical_coordinates#Vector_fields_2
      double sinTheta = sin(normalTheta);
      estimatedNormal_ = LinearAlgebra::CreateVector(sinTheta * cos(normalPhi), sinTheta * sin(normalPhi), cos(normalTheta));
    }
      
    std::vector<double> polygonsProjection;
    polygonsProjection.reserve(countPolygons);
    
    for (size_t i = 0; i < structures_.size(); i++)
    {
      const Polygons& polygons = structures_[i].polygons_;

      for (Polygons::const_iterator it = polygons.begin(); it != polygons.end(); ++it)
      {
        const Points& points = it->GetPoints();
        polygonsProjection.push_back(GeometryToolbox::ProjectAlongNormal(points[0], estimatedNormal_));
      }
    }

    std::sort(polygonsProjection.begin(), polygonsProjection.end());

    std::vector<double> deltas;
    deltas.reserve(polygonsProjection.size());

    for (size_t i = 0; i + 1 < polygonsProjection.size(); i++)
    {
      if (!LinearAlgebra::IsNear(polygonsProjection[i], polygonsProjection[i + 1]))
      {
        assert(polygonsProjection[i + 1] > polygonsProjection[i]);
        deltas.push_back(polygonsProjection[i + 1] - polygonsProjection[i]);
      }
    }

    if (deltas.empty())
    {
      estimatedSliceThickness_ = 1;
    }
    else
    {
      estimatedSliceThickness_ = LinearAlgebra::ComputeMedian(deltas);
    }
  }
}
