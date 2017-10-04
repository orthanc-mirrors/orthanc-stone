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

#include "../Enumerations.h"

#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

#include <boost/lexical_cast.hpp>

namespace OrthancStone
{
  static bool ParseDouble(double& target,
                          const std::string& source)
  {
    try
    {
      target = boost::lexical_cast<double>(source);
      return true;
    }
    catch (boost::bad_lexical_cast&)
    {
      return false;
    }
  }
  
  bool Slice::ComputeRTDoseGeometry(const Orthanc::DicomMap& dataset,
                                    unsigned int frame)
  {
    // http://dicom.nema.org/medical/Dicom/2016a/output/chtml/part03/sect_C.8.8.3.2.html

    std::string increment, offsetTag;

    if (!dataset.CopyToString(increment, Orthanc::DICOM_TAG_FRAME_INCREMENT_POINTER, false) ||
        !dataset.CopyToString(offsetTag, Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR, false))
    {
      LOG(ERROR) << "Cannot read the \"GridFrameOffsetVector\" tag, check you are using Orthanc >= 1.3.1";
      return false;
    }

    Orthanc::Toolbox::ToUpperCase(increment);
    if (increment != "3004,000C" ||
        offsetTag.empty())
    {
      return false;
    }

    std::vector<std::string> offsets;
    Orthanc::Toolbox::TokenizeString(offsets, offsetTag, '\\');

    if (frameCount_ <= 1 ||
        offsets.size() != frameCount_ ||
        frame >= frameCount_)
    {
      LOG(ERROR) << "No information about the 3D location of some slice(s) in a RT DOSE";
      return false;
    }

    double offset0, offset1, z;

    if (!ParseDouble(offset0, offsets[0]) ||
        !ParseDouble(offset1, offsets[1]) ||
        !ParseDouble(z, offsets[frame]))
    {
      LOG(ERROR) << "Invalid syntax";
      return false;
    }

    if (!GeometryToolbox::IsCloseToZero(offset0))
    {
      LOG(ERROR) << "Invalid syntax";
      return false;
    }

    geometry_ = CoordinateSystem3D(geometry_.GetOrigin() + z * geometry_.GetNormal(),
                                   //+ 650 * geometry_.GetAxisX(),
                                   geometry_.GetAxisX(),
                                   geometry_.GetAxisY());

    thickness_ = offset1 - offset0;
    if (thickness_ < 0)
    {
      thickness_ = -thickness_;
    }

    printf("%d: %f %f %f\n", frame_, geometry_.GetOrigin()[0], geometry_.GetOrigin()[1], geometry_.GetOrigin()[2]);
    //printf("%f %f %f\n", pixelSpacingX_, pixelSpacingY_, thickness_);
    
    return true;
  }

  
  bool Slice::ParseOrthancFrame(const Orthanc::DicomMap& dataset,
                                const std::string& instanceId,
                                unsigned int frame)
  {
    orthancInstanceId_ = instanceId;
    frame_ = frame;
    type_ = Type_OrthancDecodableFrame;
    imageInformation_.reset(new Orthanc::DicomImageInformation(dataset));

    if (!dataset.CopyToString(sopClassUid_, Orthanc::DICOM_TAG_SOP_CLASS_UID, false) ||
        sopClassUid_.empty())
    {
      LOG(ERROR) << "Instance without a SOP class UID";
      return false; 
    }

    if (!dataset.ParseUnsignedInteger32(frameCount_, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES))
    {
      frameCount_ = 1;   // Assume instance with one frame
    }

    if (frame >= frameCount_)
    {
      return false;
    }

    if (!dataset.ParseUnsignedInteger32(width_, Orthanc::DICOM_TAG_COLUMNS) ||
        !dataset.ParseUnsignedInteger32(height_, Orthanc::DICOM_TAG_ROWS))
    {
      return false;
    }

    thickness_ = 100.0 * std::numeric_limits<double>::epsilon();

    std::string tmp;
    if (dataset.CopyToString(tmp, Orthanc::DICOM_TAG_SLICE_THICKNESS, false))
    {
      if (!tmp.empty() &&
          !ParseDouble(thickness_, tmp))
      {
        return false;  // Syntax error
      }
    }
    
    converter_.ReadParameters(dataset);

    GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, dataset);

    std::string position, orientation;
    if (dataset.CopyToString(position, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT, false) &&
        dataset.CopyToString(orientation, Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT, false))
    {
      geometry_ = CoordinateSystem3D(position, orientation);

      bool ok = true;
      SopClassUid tmp;

      if (StringToSopClassUid(tmp, sopClassUid_))
      {
        switch (tmp)
        {
          case SopClassUid_RTDose:
            type_ = Type_OrthancRawFrame;
            ok = ComputeRTDoseGeometry(dataset, frame);
            break;
            
          default:
            break;
        }
      }

      if (!ok)
      {
        LOG(ERROR) << "Cannot deduce the 3D location of frame " << frame
                   << " in instance " << instanceId << ", whose SOP class UID is: " << sopClassUid_;
        return false;
      }
    }

    return true;
  }

  
  const std::string Slice::GetOrthancInstanceId() const
  {
    if (type_ == Type_OrthancDecodableFrame ||
        type_ == Type_OrthancRawFrame)
    {
      return orthancInstanceId_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }   
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


  const Orthanc::DicomImageInformation& Slice::GetImageInformation() const
  {
    if (imageInformation_.get() == NULL)
    {
      // Only available if constructing the "Slice" object with a DICOM map
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      return *imageInformation_;
    }
  }
}
