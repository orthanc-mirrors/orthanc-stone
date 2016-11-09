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
#include "../Messaging/MessagingToolbox.h"

#include <stdio.h>
#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  static const Json::Value& GetSequence(const Json::Value& instance,
                                        uint16_t group,
                                        uint16_t element)
  {
    char buf[16];
    sprintf(buf, "%04x,%04x", group, element);

    if (instance.type() != Json::objectValue ||
        !instance.isMember(buf) ||
        instance[buf].type() != Json::objectValue ||
        !instance[buf].isMember("Type") ||
        !instance[buf].isMember("Value") ||
        instance[buf]["Type"].type() != Json::stringValue ||
        instance[buf]["Value"].type() != Json::arrayValue ||
        instance[buf]["Type"].asString() != "Sequence")
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    return instance[buf]["Value"];
  }


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
                                                        IOrthancConnection& orthanc,
                                                        const Json::Value& contour)
  {
    const Json::Value& sequence = GetSequence(contour, 0x3006, 0x0016);

    if (sequence.size() != 1)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
    }

    DicomDataset contourImageSequence(sequence[0]);
    std::string parentUid = contourImageSequence.GetStringValue(std::make_pair(0x0008, 0x1155));

    std::string post;
    orthanc.RestApiPost(post, "/tools/lookup", parentUid);

    Json::Value tmp;
    MessagingToolbox::ParseJson(tmp, post);

    if (tmp.type() != Json::arrayValue ||
        tmp.size() != 1 ||
        !tmp[0].isMember("Type") ||
        !tmp[0].isMember("Path") ||
        tmp[0]["Type"].type() != Json::stringValue ||
        tmp[0]["ID"].type() != Json::stringValue ||
        tmp[0]["Type"].asString() != "Instance")
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);          
    }

    Json::Value parentInstance;
    MessagingToolbox::RestApiGet(parentInstance, orthanc, "/instances/" + tmp[0]["ID"].asString());

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

    Json::Value parentTags;
    MessagingToolbox::RestApiGet(parentTags, orthanc, "/instances/" + tmp[0]["ID"].asString() + "/tags?simplify");
      
    if (parentTags.type() != Json::objectValue ||
        !parentTags.isMember("ImageOrientationPatient") ||
        !parentTags.isMember("ImagePositionPatient") ||
        parentTags["ImageOrientationPatient"].type() != Json::stringValue ||
        parentTags["ImagePositionPatient"].type() != Json::stringValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NetworkProtocol);          
    }

    SliceGeometry slice(parentTags["ImagePositionPatient"].asString(),
                        parentTags["ImageOrientationPatient"].asString());
                          
    sliceThickness = 1;  // 1 mm by default

    if (parentTags.isMember("SliceThickness") &&
        parentTags["SliceThickness"].type() == Json::stringValue)
    {
      Vector tmp;
      GeometryToolbox::ParseVector(tmp, parentTags["SliceThickness"].asString());
      if (tmp.size() > 0)
      {
        sliceThickness = tmp[0];
      }
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


  DicomStructureSet::DicomStructureSet(IOrthancConnection& orthanc,
                                       const std::string& instanceId)
  {
    Json::Value instance;
    MessagingToolbox::RestApiGet(instance, orthanc, "/instances/" + instanceId + "/tags");
 
    Json::Value rtRoiObservationSequence = GetSequence(instance, 0x3006, 0x0080);
    Json::Value roiContourSequence = GetSequence(instance, 0x3006, 0x0039);
    Json::Value structureSetRoiSequence = GetSequence(instance, 0x3006, 0x0020);

    Json::Value::ArrayIndex count = rtRoiObservationSequence.size();
    if (count != roiContourSequence.size() ||
        count != structureSetRoiSequence.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
      
    structures_.resize(count);
    for (Json::Value::ArrayIndex i = 0; i < count; i++)
    {
      DicomDataset observation(rtRoiObservationSequence[i]);
      DicomDataset roi(structureSetRoiSequence[i]);
      DicomDataset content(roiContourSequence[i]);

      structures_[i].interpretation_ = observation.GetStringValue(std::make_pair(0x3006, 0x00a4), "No interpretation");
      structures_[i].name_ = roi.GetStringValue(std::make_pair(0x3006, 0x0026), "No name");
      structures_[i].red_ = 255;
      structures_[i].green_ = 0;
      structures_[i].blue_ = 0;

      DicomDataset::Tag tag(0x3006, 0x002a);

      Vector color;
      if (content.HasTag(tag))
      {
        content.GetVectorValue(color, tag);
        if (color.size() == 3)
        {
          structures_[i].red_ = ConvertColor(color[0]);
          structures_[i].green_ = ConvertColor(color[1]);
          structures_[i].blue_ = ConvertColor(color[2]);
        }
      }

      const Json::Value& slices = GetSequence(roiContourSequence[i], 0x3006, 0x0040);

      LOG(WARNING) << "New RT structure: \"" << structures_[i].name_ 
                   << "\" with interpretation \"" << structures_[i].interpretation_
                   << "\" containing " << slices.size() << " slices (color: " 
                   << static_cast<int>(structures_[i].red_) << "," 
                   << static_cast<int>(structures_[i].green_) << ","
                   << static_cast<int>(structures_[i].blue_) << ")";

      for (Json::Value::ArrayIndex j = 0; j < slices.size(); j++)
      {
        DicomDataset slice(slices[j]);

        unsigned int npoints = slice.GetUnsignedIntegerValue(std::make_pair(0x3006, 0x0046));
        LOG(INFO) << "Parsing slice containing " << npoints << " vertices";

        if (slice.GetStringValue(std::make_pair(0x3006, 0x0042)) != "CLOSED_PLANAR")
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
        }

        std::string slicesData;
        orthanc.RestApiGet(slicesData, "/instances/" + instanceId + "/content/3006-0039/" +
                           boost::lexical_cast<std::string>(i) + "/3006-0040/" +
                           boost::lexical_cast<std::string>(j) + "/3006-0050");

        Vector points;
        if (!GeometryToolbox::ParseVector(points, slicesData) ||
            points.size() != 3 * npoints)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);          
        }

        Polygon polygon;
        SliceGeometry geometry = ExtractSliceGeometry(polygon.sliceThickness_, orthanc, slices[j]);
        polygon.projectionAlongNormal_ = geometry.ProjectAlongNormal(geometry.GetOrigin());

        for (size_t k = 0; k < npoints; k++)
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
