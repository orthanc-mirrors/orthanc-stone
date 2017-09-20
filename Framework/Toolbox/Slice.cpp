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


#include "Slice.h"

#include <Core/OrthancException.h>

namespace OrthancStone
{
  bool Slice::ParseOrthancFrame(const OrthancPlugins::IDicomDataset& dataset,
                                const std::string& instanceId,
                                unsigned int frame)
  {
    OrthancPlugins::DicomDatasetReader reader(dataset);

    unsigned int frameCount;
    if (!reader.GetUnsignedIntegerValue(frameCount, OrthancPlugins::DICOM_TAG_NUMBER_OF_FRAMES))
    {
      frameCount = 1;   // Assume instance with one frame
    }

    if (frame >= frameCount)
    {
      return false;
    }

    if (!reader.GetDoubleValue(thickness_, OrthancPlugins::DICOM_TAG_SLICE_THICKNESS))
    {
      thickness_ = 100.0 * std::numeric_limits<double>::epsilon();
    }

    GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, dataset);

    std::string position, orientation;
    if (dataset.GetStringValue(position, OrthancPlugins::DICOM_TAG_IMAGE_POSITION_PATIENT) &&
        dataset.GetStringValue(orientation, OrthancPlugins::DICOM_TAG_IMAGE_ORIENTATION_PATIENT))
    {
      geometry_ = CoordinateSystem3D(position, orientation);
    }
      
    if (reader.GetUnsignedIntegerValue(width_, OrthancPlugins::DICOM_TAG_COLUMNS) &&
        reader.GetUnsignedIntegerValue(height_, OrthancPlugins::DICOM_TAG_ROWS))
    {
      orthancInstanceId_ = instanceId;
      frame_ = frame;
      converter_.ReadParameters(dataset);

      type_ = Type_OrthancInstance;
      return true;
    }
    else
    {
      return false;
    }
  }

  
  const std::string Slice::GetOrthancInstanceId() const
  {
    if (type_ != Type_OrthancInstance)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return orthancInstanceId_;
  }

  
  unsigned int Slice::GetFrame() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return frame_;
  }

  
  const CoordinateSystem3D& Slice::GetGeometry() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return geometry_;
  }

  
  double Slice::GetThickness() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return thickness_;
  }

  
  double Slice::GetPixelSpacingX() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return pixelSpacingX_;
  }

  
  double Slice::GetPixelSpacingY() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return pixelSpacingY_;
  }

  
  unsigned int Slice::GetWidth() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return width_;
  }

  
  unsigned int Slice::GetHeight() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return height_;
  }


  const DicomFrameConverter& Slice::GetConverter() const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
        
    return converter_;
  }


  bool Slice::ContainsPlane(const CoordinateSystem3D& plane) const
  {
    if (type_ == Type_Invalid)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }

    bool opposite;
    return (GeometryToolbox::IsParallelOrOpposite(opposite,
                                                  GetGeometry().GetNormal(),
                                                  plane.GetNormal()) &&
            GeometryToolbox::IsNear(GetGeometry().ProjectAlongNormal(GetGeometry().GetOrigin()),
                                    GetGeometry().ProjectAlongNormal(plane.GetOrigin()),
                                    thickness_ / 2.0));
  }

  
  void Slice::GetExtent(std::vector<Vector>& points) const
  {
    double sx = GetPixelSpacingX();
    double sy = GetPixelSpacingY();
    double w = static_cast<double>(GetWidth());
    double h = static_cast<double>(GetHeight());

    points.clear();
    points.push_back(GetGeometry().MapSliceToWorldCoordinates(-0.5      * sx, -0.5      * sy));
    points.push_back(GetGeometry().MapSliceToWorldCoordinates((w - 0.5) * sx, -0.5      * sy));
    points.push_back(GetGeometry().MapSliceToWorldCoordinates(-0.5      * sx, (h - 0.5) * sy));
    points.push_back(GetGeometry().MapSliceToWorldCoordinates((w - 0.5) * sx, (h - 0.5) * sy));
  }
}
