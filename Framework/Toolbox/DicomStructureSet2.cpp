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

#include "DicomStructureSet2.h"

#include "../Toolbox/LinearAlgebra.h"
#include "../StoneException.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>
#include <Core/DicomFormat/DicomTag.h>

#include <Plugins/Samples/Common/FullOrthancDataset.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>

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

  static inline uint8_t ConvertAndClipToByte(double v)
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

  static bool ReadDicomToVector(Vector& target,
    const OrthancPlugins::IDicomDataset& dataset,
    const OrthancPlugins::DicomPath& tag)
  {
    std::string value;
    return (dataset.GetStringValue(value, tag) &&
      LinearAlgebra::ParseVector(target, value));
  }
  
  std::ostream& operator<<(std::ostream& s, const OrthancPlugins::DicomPath& dicomPath)
  {
    std::stringstream tmp;
    for (size_t i = 0; i < dicomPath.GetPrefixLength(); ++i)
    {
      OrthancPlugins::DicomTag tag = dicomPath.GetPrefixTag(i);
      
      // We use this other object to be able to use GetMainTagsName
      // and Format
      Orthanc::DicomTag tag2(tag.GetGroup(), tag.GetElement());
      size_t index = dicomPath.GetPrefixIndex(i);
      tmp << tag2.GetMainTagsName() << " (" << tag2.Format() << ") [" << index << "] / ";
    }
    const OrthancPlugins::DicomTag& tag = dicomPath.GetFinalTag();
    Orthanc::DicomTag tag2(tag.GetGroup(), tag.GetElement());
    tmp << tag2.GetMainTagsName() << " (" << tag2.Format() << ")";
    s << tmp.str();
    return s;
  }


  void DicomStructureSet2::SetContents(const OrthancPlugins::FullOrthancDataset& tags)
  {
    FillStructuresFromDataset(tags);
    ComputeDependentProperties();
  }

  void DicomStructureSet2::ComputeDependentProperties()
  {
    for (size_t i = 0; i < structures_.size(); ++i)
    {
      structures_[i].ComputeDependentProperties();
    }
  }

  void DicomStructureSet2::FillStructuresFromDataset(const OrthancPlugins::FullOrthancDataset& tags)
  {
    OrthancPlugins::DicomDatasetReader reader(tags);

    // a few sanity checks
    size_t count = 0, tmp = 0;

    //  DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE (0x3006, 0x0080);
    //  DICOM_TAG_ROI_CONTOUR_SEQUENCE         (0x3006, 0x0039);
    //  DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE   (0x3006, 0x0020);
    if (!tags.GetSequenceSize(count, DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE) ||
      !tags.GetSequenceSize(tmp, DICOM_TAG_ROI_CONTOUR_SEQUENCE) ||
      tmp != count ||
      !tags.GetSequenceSize(tmp, DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE) ||
      tmp != count)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    // let's now parse the structures stored in the dicom file
    //  DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE  (0x3006, 0x0080)
    //  DICOM_TAG_RT_ROI_INTERPRETED_TYPE       (0x3006, 0x00a4)
    //  DICOM_TAG_ROI_DISPLAY_COLOR             (0x3006, 0x002a)
    //  DICOM_TAG_ROI_NAME                      (0x3006, 0x0026)
    structures_.resize(count);
    for (size_t i = 0; i < count; i++)
    {
      // (0x3006, 0x0080)[i]/(0x3006, 0x00a4)
      structures_[i].interpretation_ = reader.GetStringValue
      (OrthancPlugins::DicomPath(DICOM_TAG_RT_ROI_OBSERVATIONS_SEQUENCE, i,
        DICOM_TAG_RT_ROI_INTERPRETED_TYPE),
        "No interpretation");

      // (0x3006, 0x0020)[i]/(0x3006, 0x0026)
      structures_[i].name_ = reader.GetStringValue
      (OrthancPlugins::DicomPath(DICOM_TAG_STRUCTURE_SET_ROI_SEQUENCE, i,
        DICOM_TAG_ROI_NAME),
        "No name");

      Vector color;
      // (0x3006, 0x0039)[i]/(0x3006, 0x002a)
      if (ReadDicomToVector(color, tags, OrthancPlugins::DicomPath(
        DICOM_TAG_ROI_CONTOUR_SEQUENCE, i, DICOM_TAG_ROI_DISPLAY_COLOR)) 
        && color.size() == 3)
      {
        structures_[i].red_   = ConvertAndClipToByte(color[0]);
        structures_[i].green_ = ConvertAndClipToByte(color[1]);
        structures_[i].blue_  = ConvertAndClipToByte(color[2]);
      }
      else
      {
        structures_[i].red_ = 255;
        structures_[i].green_ = 0;
        structures_[i].blue_ = 0;
      }

      size_t countSlices;
      //  DICOM_TAG_ROI_CONTOUR_SEQUENCE          (0x3006, 0x0039);
      //  DICOM_TAG_CONTOUR_SEQUENCE              (0x3006, 0x0040);
      if (!tags.GetSequenceSize(countSlices, OrthancPlugins::DicomPath(
        DICOM_TAG_ROI_CONTOUR_SEQUENCE, i, DICOM_TAG_CONTOUR_SEQUENCE)))
      {
        LOG(WARNING) << "DicomStructureSet2::SetContents | structure \"" << structures_[i].name_ << "\" has no slices!";
        countSlices = 0;
      }

      LOG(INFO) << "New RT structure: \"" << structures_[i].name_
        << "\" with interpretation \"" << structures_[i].interpretation_
        << "\" containing " << countSlices << " slices (color: "
        << static_cast<int>(structures_[i].red_) << ","
        << static_cast<int>(structures_[i].green_) << ","
        << static_cast<int>(structures_[i].blue_) << ")";

      // These temporary variables avoid allocating many vectors in the loop below
      
      // (0x3006, 0x0039)[i]/(0x3006, 0x0040)[0]/(0x3006, 0x0046)
      OrthancPlugins::DicomPath countPointsPath(
        DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
        DICOM_TAG_CONTOUR_SEQUENCE, 0,
        DICOM_TAG_NUMBER_OF_CONTOUR_POINTS);

      OrthancPlugins::DicomPath geometricTypePath(
        DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
        DICOM_TAG_CONTOUR_SEQUENCE, 0,
        DICOM_TAG_CONTOUR_GEOMETRIC_TYPE);

      OrthancPlugins::DicomPath imageSequencePath(
        DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
        DICOM_TAG_CONTOUR_SEQUENCE, 0,
        DICOM_TAG_CONTOUR_IMAGE_SEQUENCE);

      // (3006,0039)[i] / (0x3006, 0x0040)[0] / (0x3006, 0x0016)[0] / (0x0008, 0x1155)
      OrthancPlugins::DicomPath referencedInstancePath(
        DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
        DICOM_TAG_CONTOUR_SEQUENCE, 0,
        DICOM_TAG_CONTOUR_IMAGE_SEQUENCE, 0,
        DICOM_TAG_REFERENCED_SOP_INSTANCE_UID);

      OrthancPlugins::DicomPath contourDataPath(
        DICOM_TAG_ROI_CONTOUR_SEQUENCE, i,
        DICOM_TAG_CONTOUR_SEQUENCE, 0,
        DICOM_TAG_CONTOUR_DATA);

      for (size_t j = 0; j < countSlices; j++)
      {
        unsigned int countPoints = 0;

        countPointsPath.SetPrefixIndex(1, j);
        if (!reader.GetUnsignedIntegerValue(countPoints, countPointsPath))
        {
          LOG(ERROR) << "Dicom path " << countPointsPath << " is not valid (should contain an unsigned integer)";
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }

        //LOG(INFO) << "Parsing slice containing " << countPoints << " vertices";

        geometricTypePath.SetPrefixIndex(1, j);
        std::string type = reader.GetMandatoryStringValue(geometricTypePath);
        if (type != "CLOSED_PLANAR")
        {
          // TODO: support points!!
          LOG(WARNING) << "Ignoring contour with geometry type: " << type;
          continue;
        }

        size_t size = 0;

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
        if (Orthanc::Toolbox::StripSpaces(sopInstanceUid) == "")
        {
          LOG(ERROR) << "WARNING. The following Dicom tag (Referenced SOP Instance UID) contains an empty value : // (3006,0039)[" << i << "] / (0x3006, 0x0040)[0] / (0x3006, 0x0016)[0] / (0x0008, 0x1155)";
        }

        DicomStructurePolygon2 polygon(sopInstanceUid,type);
        polygon.Reserve(countPoints);

        for (size_t k = 0; k < countPoints; k++)
        {
          Vector v(3);
          v[0] = points[3 * k];
          v[1] = points[3 * k + 1];
          v[2] = points[3 * k + 2];
          polygon.AddPoint(v);
        }
        structures_[i].AddPolygon(polygon);
      }
    }
  }


  void DicomStructureSet2::Clear()
  {
    structures_.clear();
  }

}