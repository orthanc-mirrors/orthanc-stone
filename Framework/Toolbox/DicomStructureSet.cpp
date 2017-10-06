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


#include "DicomStructureSet.h"

#include "../Toolbox/MessagingToolbox.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Plugins/Samples/Common/FullOrthancDataset.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>

#include <stdio.h>
#include <boost/lexical_cast.hpp>

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
            GeometryToolbox::ParseVector(target, value));
  }


  void DicomStructureSet::Polygon::CheckPoint(const Vector& v)
  {
    if (hasSlice_)
    {
      if (!GeometryToolbox::IsNear(GeometryToolbox::ProjectAlongNormal(v, normal_),
                                   projectionAlongNormal_, 
                                   sliceThickness_ / 2.0 /* in mm */))
      {
        LOG(ERROR) << "This RT-STRUCT contains a point that is off the slice of its instance";
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
      }
    }
  }


  void DicomStructureSet::Polygon::AddPoint(const Vector& v)
  {
    CheckPoint(v);
    points_.push_back(v);
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
        normal_ = geometry.GetNormal();
        projectionAlongNormal_ = GeometryToolbox::ProjectAlongNormal(geometry.GetOrigin(), normal_);
        sliceThickness_ = it->second.thickness_;

        for (Points::const_iterator it = points_.begin(); it != points_.end(); ++it)
        {
          CheckPoint(*it);
        }

        return true;
      }
    }
  }


  bool DicomStructureSet::Polygon::IsOnSlice(const CoordinateSystem3D& slice) const
  {
    bool isOpposite;

    if (points_.empty() ||
        !hasSlice_ ||
        !GeometryToolbox::IsParallelOrOpposite(isOpposite, slice.GetNormal(), normal_))
    {
      return false;
    }
    
    double d = GeometryToolbox::ProjectAlongNormal(slice.GetOrigin(), normal_);

    return (GeometryToolbox::IsNear(d, projectionAlongNormal_,
                                    sliceThickness_ / 2.0));
  }

  
  const DicomStructureSet::Structure& DicomStructureSet::GetStructure(size_t index) const
  {
    if (index >= structures_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    return structures_[index];
  }


  DicomStructureSet::DicomStructureSet(const OrthancPlugins::FullOrthancDataset& tags)
  {
    using namespace OrthancPlugins;

    DicomDatasetReader reader(tags);
    
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
        (DicomPath(DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE, i,
                   DICOM_TAG_RT_ROI_INTERPRETED_TYPE),
         "No interpretation");

      structures_[i].name_ = reader.GetStringValue
        (DicomPath(DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE, i,
                   DICOM_TAG_ROI_NAME),
         "No interpretation");

      Vector color;
      if (ParseVector(color, tags, DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
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
      if (!tags.GetSequenceSize(countSlices, DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                       DICOM_TAG_CONTOUR_SEQUENCE)))
      {
        countSlices = 0;
      }

      LOG(WARNING) << "New RT structure: \"" << structures_[i].name_ 
                   << "\" with interpretation \"" << structures_[i].interpretation_
                   << "\" containing " << countSlices << " slices (color: " 
                   << static_cast<int>(structures_[i].red_) << "," 
                   << static_cast<int>(structures_[i].green_) << ","
                   << static_cast<int>(structures_[i].blue_) << ")";

      for (size_t j = 0; j < countSlices; j++)
      {
        unsigned int countPoints;

        if (!reader.GetUnsignedIntegerValue
            (countPoints, DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                    DICOM_TAG_CONTOUR_SEQUENCE, j,
                                    DICOM_TAG_NUMBER_OF_CONTOUR_POINTS)))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
            
        //LOG(INFO) << "Parsing slice containing " << countPoints << " vertices";

        std::string type = reader.GetMandatoryStringValue
          (DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                     DICOM_TAG_CONTOUR_SEQUENCE, j,
                     DICOM_TAG_CONTOUR_GEOMETRIC_TYPE));
        if (type != "CLOSED_PLANAR")
        {
          LOG(ERROR) << "Cannot handle contour with geometry type: " << type;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
        }

        size_t size;
        if (!tags.GetSequenceSize(size, DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                  DICOM_TAG_CONTOUR_SEQUENCE, j,
                                                  DICOM_TAG_CONTOUR_IMAGE_SEQUENCE)) ||
            size != 1)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
        }

        std::string sopInstanceUid = reader.GetMandatoryStringValue
          (DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                     DICOM_TAG_CONTOUR_SEQUENCE, j,
                     DICOM_TAG_CONTOUR_IMAGE_SEQUENCE, 0,
                     DICOM_TAG_REFERENCED_SOP_INSTANCE_UID));
        
        std::string slicesData = reader.GetMandatoryStringValue
          (DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                     DICOM_TAG_CONTOUR_SEQUENCE, j,
                     DICOM_TAG_CONTOUR_DATA));

        Vector points;
        if (!GeometryToolbox::ParseVector(points, slicesData) ||
            points.size() != 3 * countPoints)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
        }

        Polygon polygon(sopInstanceUid);

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
    GeometryToolbox::AssignVector(center, 0, 0, 0);
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


  void DicomStructureSet::Render(CairoContext& context,
                                 const CoordinateSystem3D& slice)
  {
    cairo_t* cr = context.GetObject();

    for (Structures::iterator structure = structures_.begin();
         structure != structures_.end(); ++structure)
    {
      for (Polygons::iterator polygon = structure->polygons_.begin();
           polygon != structure->polygons_.end(); ++polygon)
      {
        polygon->UpdateReferencedSlice(referencedSlices_);
          
        if (polygon->IsOnSlice(slice))
        {
          context.SetSourceColor(structure->red_, structure->green_, structure->blue_);

          Points::const_iterator p = polygon->GetPoints().begin();

          double x, y;
          slice.ProjectPoint(x, y, *p);
          cairo_move_to(cr, x, y);
          ++p;
            
          while (p != polygon->GetPoints().end())
          {
            slice.ProjectPoint(x, y, *p);
            cairo_line_to(cr, x, y);
            ++p;
          }

          slice.ProjectPoint(x, y, *polygon->GetPoints().begin());
          cairo_line_to(cr, x, y);

          cairo_stroke(cr);
        }
      }
    }
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
    }
  }


  void DicomStructureSet::AddReferencedSlice(const Orthanc::DicomMap& dataset)
  {
    CoordinateSystem3D slice(dataset);

    double thickness = 1;  // 1 mm by default

    std::string s;
    Vector v;
    if (dataset.CopyToString(s, Orthanc::DICOM_TAG_SLICE_THICKNESS, false) &&
        GeometryToolbox::ParseVector(v, s) &&
        v.size() > 0)
    {
      thickness = v[0];
    }

    std::string instance, series;
    if (dataset.CopyToString(instance, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false) &&
        dataset.CopyToString(series, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false))
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
          LOG(ERROR) << "Missing information about referenced instance: "
                     << polygon->GetSopInstanceUid();
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
        }
      }
    }
  }


  Vector DicomStructureSet::GetNormal() const
  {
    if (!referencedSlices_.empty())
    {
      Vector v;
      GeometryToolbox::AssignVector(v, 0, 0, 1);
      return v;
    }
    else
    {
      return referencedSlices_.begin()->second.geometry_.GetNormal();
    }
  }

  
  DicomStructureSet* DicomStructureSet::SynchronousLoad(OrthancPlugins::IOrthancConnection& orthanc,
                                                        const std::string& instanceId)
  {
    const std::string uri = "/instances/" + instanceId + "/tags?ignore-length=3006-0050";
    OrthancPlugins::FullOrthancDataset dataset(orthanc, uri);

    std::auto_ptr<DicomStructureSet> result(new DicomStructureSet(dataset));

    std::set<std::string> instances;
    result->GetReferencedInstances(instances);

    for (std::set<std::string>::const_iterator it = instances.begin();
         it != instances.end(); ++it)
    {
      Json::Value lookup;
      MessagingToolbox::RestApiPost(lookup, orthanc, "/tools/lookup", *it);

      if (lookup.type() != Json::arrayValue ||
          lookup.size() != 1 ||
          !lookup[0].isMember("Type") ||
          !lookup[0].isMember("Path") ||
          lookup[0]["Type"].type() != Json::stringValue ||
          lookup[0]["ID"].type() != Json::stringValue ||
          lookup[0]["Type"].asString() != "Instance")
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);          
      }

      OrthancPlugins::FullOrthancDataset slice
        (orthanc, "/instances/" + lookup[0]["ID"].asString() + "/tags");
      Orthanc::DicomMap m;
      MessagingToolbox::ConvertDataset(m, slice);
      result->AddReferencedSlice(m);
    }

    result->CheckReferencedSlices();

    return result.release();
  }

}
