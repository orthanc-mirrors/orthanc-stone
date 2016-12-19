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


#include "DicomStructureSet.h"

#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"
#include "../../Resources/Orthanc/Plugins/Samples/Common/FullOrthancDataset.h"
#include "../Toolbox/MessagingToolbox.h"

#include <stdio.h>
#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  static const OrthancPlugins::DicomTag DICOM_TAG_CONTOUR_GEOMETRIC_TYPE(0x3006, 0x0042);
  static const OrthancPlugins::DicomTag DICOM_TAG_CONTOUR_IMAGE_SEQUENCE(0x3006, 0x0016);
  static const OrthancPlugins::DicomTag DICOM_TAG_CONTOUR_SEQUENCE(0x3006, 0x0040);
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


  SliceGeometry DicomStructureSet::ExtractSliceGeometry(double& sliceThickness,
                                                        OrthancPlugins::IOrthancConnection& orthanc,
                                                        const OrthancPlugins::IDicomDataset& tags,
                                                        size_t contourIndex,
                                                        size_t sliceIndex)
  {
    using namespace OrthancPlugins;

    size_t size;
    if (!tags.GetSequenceSize(size, DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, contourIndex,
                                              DICOM_TAG_CONTOUR_SEQUENCE, sliceIndex,
                                              DICOM_TAG_CONTOUR_IMAGE_SEQUENCE)) ||
        size != 1)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
    }

    DicomDatasetReader reader(tags);
    std::string parentUid = reader.GetMandatoryStringValue(DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, contourIndex,
                                                                     DICOM_TAG_CONTOUR_SEQUENCE, sliceIndex,
                                                                     DICOM_TAG_CONTOUR_IMAGE_SEQUENCE, 0,
                                                                     DICOM_TAG_REFERENCED_SOP_INSTANCE_UID));

    Json::Value parentLookup;
    MessagingToolbox::RestApiPost(parentLookup, orthanc, "/tools/lookup", parentUid);

    if (parentLookup.type() != Json::arrayValue ||
        parentLookup.size() != 1 ||
        !parentLookup[0].isMember("Type") ||
        !parentLookup[0].isMember("Path") ||
        parentLookup[0]["Type"].type() != Json::stringValue ||
        parentLookup[0]["ID"].type() != Json::stringValue ||
        parentLookup[0]["Type"].asString() != "Instance")
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);          
    }

    Json::Value parentInstance;
    MessagingToolbox::RestApiGet(parentInstance, orthanc, "/instances/" + parentLookup[0]["ID"].asString());

    if (parentInstance.type() != Json::objectValue ||
        !parentInstance.isMember("ParentSeries") ||
        parentInstance["ParentSeries"].type() != Json::stringValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);          
    }

    std::string parentSeriesId = parentInstance["ParentSeries"].asString();
    bool isFirst = parentSeriesId_.empty();

    if (isFirst)
    {
      parentSeriesId_ = parentSeriesId;
    }
    else if (parentSeriesId_ != parentSeriesId)
    {
      LOG(ERROR) << "This RT-STRUCT refers to several different series";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    FullOrthancDataset parentTags(orthanc, "/instances/" + parentLookup[0]["ID"].asString() + "/tags");
    SliceGeometry slice(parentTags);

    Vector v;
    if (GeometryToolbox::ParseVector(v, parentTags, DICOM_TAG_SLICE_THICKNESS) &&
        v.size() > 0)
    {
      sliceThickness = v[0];
    }
    else
    {
      sliceThickness = 1;  // 1 mm by default
    }

    if (isFirst)
    {
      normal_ = slice.GetNormal();
    }
    else if (!GeometryToolbox::IsParallel(normal_, slice.GetNormal()))
    {
      LOG(ERROR) << "Incompatible orientation of slices in this RT-STRUCT";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    return slice;
  }


  const DicomStructureSet::Structure& DicomStructureSet::GetStructure(size_t index) const
  {
    if (index >= structures_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    return structures_[index];
  }


  bool DicomStructureSet::IsPolygonOnSlice(const Polygon& polygon,
                                           const SliceGeometry& geometry) const
  {
    double d = boost::numeric::ublas::inner_prod(geometry.GetOrigin(), normal_);

    return (GeometryToolbox::IsNear(d, polygon.projectionAlongNormal_, polygon.sliceThickness_ / 2.0) &&
            !polygon.points_.empty());
  }


  DicomStructureSet::DicomStructureSet(OrthancPlugins::IOrthancConnection& orthanc,
                                       const std::string& instanceId)
  {
    using namespace OrthancPlugins;

    FullOrthancDataset tags(orthanc, "/instances/" + instanceId + "/tags");
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
      structures_[i].interpretation_ = reader.GetStringValue(DicomPath(DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE, i,
                                                                       DICOM_TAG_RT_ROI_INTERPRETED_TYPE),
                                                             "No interpretation");

      structures_[i].name_ = reader.GetStringValue(DicomPath(DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE, i,
                                                             DICOM_TAG_ROI_NAME),
                                                   "No interpretation");

      Vector color;
      if (GeometryToolbox::ParseVector(color, tags, DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
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
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
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

        if (!reader.GetUnsignedIntegerValue(countPoints, DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                                   DICOM_TAG_CONTOUR_SEQUENCE, j,
                                                                   DICOM_TAG_NUMBER_OF_CONTOUR_POINTS)))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
            
        LOG(INFO) << "Parsing slice containing " << countPoints << " vertices";

        std::string type = reader.GetMandatoryStringValue(DicomPath(DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
                                                                    DICOM_TAG_CONTOUR_SEQUENCE, j,
                                                                    DICOM_TAG_CONTOUR_GEOMETRIC_TYPE));
        if (type != "CLOSED_PLANAR")
        {
          LOG(ERROR) << "Cannot handle contour with geometry type: " << type;
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
        }

        // The "CountourData" tag (3006,0050) is too large to be
        // returned by the "/instances/{id}/tags" URI: Access it using
        // the raw "/instances/{id}/content/{...}" endpoint
        std::string slicesData;
        orthanc.RestApiGet(slicesData, "/instances/" + instanceId + "/content/3006-0039/" +
                           boost::lexical_cast<std::string>(i) + "/3006-0040/" +
                           boost::lexical_cast<std::string>(j) + "/3006-0050");

        Vector points;
        if (!GeometryToolbox::ParseVector(points, slicesData) ||
            points.size() != 3 * countPoints)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
        }

        Polygon polygon;
        SliceGeometry geometry = ExtractSliceGeometry(polygon.sliceThickness_, orthanc, tags, i, j);
        polygon.projectionAlongNormal_ = geometry.ProjectAlongNormal(geometry.GetOrigin());

        for (size_t k = 0; k < countPoints; k++)
        {
          Vector v(3);
          v[0] = points[3 * k];
          v[1] = points[3 * k + 1];
          v[2] = points[3 * k + 2];

          if (!GeometryToolbox::IsNear(geometry.ProjectAlongNormal(v), 
                                       polygon.projectionAlongNormal_, 
                                       polygon.sliceThickness_ / 2.0 /* in mm */))
          {
            LOG(ERROR) << "This RT-STRUCT contains a point that is off the slice of its instance";
            throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
          }

          polygon.points_.push_back(v);
        }

        structures_[i].polygons_.push_back(polygon);
      }
    }

    if (parentSeriesId_.empty() ||
        normal_.size() != 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
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
      if (!polygon->points_.empty())
      {
        center += polygon->points_.front() / n;
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
                                 const SliceGeometry& slice) const
  {
    cairo_t* cr = context.GetObject();

    for (Structures::const_iterator structure = structures_.begin();
         structure != structures_.end(); ++structure)
    {
      for (Polygons::const_iterator polygon = structure->polygons_.begin();
           polygon != structure->polygons_.end(); ++polygon)
      {
        if (IsPolygonOnSlice(*polygon, slice))
        {
          context.SetSourceColor(structure->red_, structure->green_, structure->blue_);

          Points::const_iterator p = polygon->points_.begin();

          double x, y;
          slice.ProjectPoint(x, y, *p);
          cairo_move_to(cr, x, y);
          ++p;
            
          while (p != polygon->points_.end())
          {
            slice.ProjectPoint(x, y, *p);
            cairo_line_to(cr, x, y);
            ++p;
          }

          slice.ProjectPoint(x, y, *polygon->points_.begin());
          cairo_line_to(cr, x, y);
        }
      }
    }

    cairo_stroke(cr);
  }
}
