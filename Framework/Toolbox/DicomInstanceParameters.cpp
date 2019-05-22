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


#include "DicomInstanceParameters.h"

#include "../Scene2D/ColorTextureSceneLayer.h"
#include "../Scene2D/FloatTextureSceneLayer.h"
#include "../Toolbox/GeometryToolbox.h"

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/Logging.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>


namespace OrthancStone
{
  void DicomInstanceParameters::Data::ComputeDoseOffsets(const Orthanc::DicomMap& dicom)
  {
    // http://dicom.nema.org/medical/Dicom/2016a/output/chtml/part03/sect_C.8.8.3.2.html

    {
      std::string increment;

      if (dicom.CopyToString(increment, Orthanc::DICOM_TAG_FRAME_INCREMENT_POINTER, false))
      {
        Orthanc::Toolbox::ToUpperCase(increment);
        if (increment != "3004,000C")  // This is the "Grid Frame Offset Vector" tag
        {
          LOG(ERROR) << "RT-DOSE: Bad value for the \"FrameIncrementPointer\" tag";
          return;
        }
      }
    }

    if (!LinearAlgebra::ParseVector(frameOffsets_, dicom, Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR) ||
        frameOffsets_.size() < imageInformation_.GetNumberOfFrames())
    {
      LOG(ERROR) << "RT-DOSE: No information about the 3D location of some slice(s)";
      frameOffsets_.clear();
    }
    else
    {
      if (frameOffsets_.size() >= 2)
      {
        thickness_ = frameOffsets_[1] - frameOffsets_[0];

        if (thickness_ < 0)
        {
          thickness_ = -thickness_;
        }
      }
    }
  }


  DicomInstanceParameters::Data::Data(const Orthanc::DicomMap& dicom) :
    imageInformation_(dicom)
  {
    if (imageInformation_.GetNumberOfFrames() <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    if (!dicom.CopyToString(studyInstanceUid_, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) ||
        !dicom.CopyToString(seriesInstanceUid_, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false) ||
        !dicom.CopyToString(sopInstanceUid_, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
        
    std::string s;
    if (!dicom.CopyToString(s, Orthanc::DICOM_TAG_SOP_CLASS_UID, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
    else
    {
      sopClassUid_ = StringToSopClassUid(s);
    }

    if (!dicom.ParseDouble(thickness_, Orthanc::DICOM_TAG_SLICE_THICKNESS))
    {
      thickness_ = 100.0 * std::numeric_limits<double>::epsilon();
    }

    GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, dicom);

    std::string position, orientation;
    if (dicom.CopyToString(position, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT, false) &&
        dicom.CopyToString(orientation, Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT, false))
    {
      geometry_ = CoordinateSystem3D(position, orientation);
    }

    if (sopClassUid_ == SopClassUid_RTDose)
    {
      ComputeDoseOffsets(dicom);
    }

    isColor_ = (imageInformation_.GetPhotometricInterpretation() != Orthanc::PhotometricInterpretation_Monochrome1 &&
                imageInformation_.GetPhotometricInterpretation() != Orthanc::PhotometricInterpretation_Monochrome2);

    double doseGridScaling;

    if (dicom.ParseDouble(rescaleIntercept_, Orthanc::DICOM_TAG_RESCALE_INTERCEPT) &&
        dicom.ParseDouble(rescaleSlope_, Orthanc::DICOM_TAG_RESCALE_SLOPE))
    {
      hasRescale_ = true;
    }
    else if (dicom.ParseDouble(doseGridScaling, Orthanc::DICOM_TAG_DOSE_GRID_SCALING))
    {
      hasRescale_ = true;
      rescaleIntercept_ = 0;
      rescaleSlope_ = doseGridScaling;
    }
    else
    {
      hasRescale_ = false;
    }

    Vector c, w;
    if (LinearAlgebra::ParseVector(c, dicom, Orthanc::DICOM_TAG_WINDOW_CENTER) &&
        LinearAlgebra::ParseVector(w, dicom, Orthanc::DICOM_TAG_WINDOW_WIDTH) &&
        c.size() > 0 && 
        w.size() > 0)
    {
      hasDefaultWindowing_ = true;
      defaultWindowingCenter_ = static_cast<float>(c[0]);
      defaultWindowingWidth_ = static_cast<float>(w[0]);
    }
    else
    {
      hasDefaultWindowing_ = false;
    }

    if (sopClassUid_ == SopClassUid_RTDose)
    {
      switch (imageInformation_.GetBitsStored())
      {
        case 16:
          expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale16;
          break;

        case 32:
          expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale32;
          break;

        default:
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      } 
    }
    else if (isColor_)
    {
      expectedPixelFormat_ = Orthanc::PixelFormat_RGB24;
    }
    else if (imageInformation_.IsSigned())
    {
      expectedPixelFormat_ = Orthanc::PixelFormat_SignedGrayscale16;
    }
    else
    {
      expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale16;
    }
  }


  CoordinateSystem3D  DicomInstanceParameters::Data::GetFrameGeometry(unsigned int frame) const
  {
    if (frame == 0)
    {
      return geometry_;
    }
    else if (frame >= imageInformation_.GetNumberOfFrames())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else if (sopClassUid_ == SopClassUid_RTDose)
    {
      if (frame >= frameOffsets_.size())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      return CoordinateSystem3D(
        geometry_.GetOrigin() + frameOffsets_[frame] * geometry_.GetNormal(),
        geometry_.GetAxisX(),
        geometry_.GetAxisY());
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }


  bool DicomInstanceParameters::Data::IsPlaneWithinSlice(unsigned int frame,
                                                         const CoordinateSystem3D& plane) const
  {
    if (frame >= imageInformation_.GetNumberOfFrames())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    CoordinateSystem3D tmp = geometry_;

    if (frame != 0)
    {
      tmp = GetFrameGeometry(frame);
    }

    double distance;

    return (CoordinateSystem3D::ComputeDistance(distance, tmp, plane) &&
            distance <= thickness_ / 2.0);
  }

      
  void DicomInstanceParameters::Data::ApplyRescale(Orthanc::ImageAccessor& image,
                                                   bool useDouble) const
  {
    if (image.GetFormat() != Orthanc::PixelFormat_Float32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }
    
    if (hasRescale_)
    {
      const unsigned int width = image.GetWidth();
      const unsigned int height = image.GetHeight();
        
      for (unsigned int y = 0; y < height; y++)
      {
        float* p = reinterpret_cast<float*>(image.GetRow(y));

        if (useDouble)
        {
          // Slower, accurate implementation using double
          for (unsigned int x = 0; x < width; x++, p++)
          {
            double value = static_cast<double>(*p);
            *p = static_cast<float>(value * rescaleSlope_ + rescaleIntercept_);
          }
        }
        else
        {
          // Fast, approximate implementation using float
          for (unsigned int x = 0; x < width; x++, p++)
          {
            *p = (*p) * static_cast<float>(rescaleSlope_) + static_cast<float>(rescaleIntercept_);
          }
        }
      }
    }
  }

  double DicomInstanceParameters::GetRescaleIntercept() const
  {
    if (data_.hasRescale_)
    {
      return data_.rescaleIntercept_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  double DicomInstanceParameters::GetRescaleSlope() const
  {
    if (data_.hasRescale_)
    {
      return data_.rescaleSlope_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  float DicomInstanceParameters::GetDefaultWindowingCenter() const
  {
    if (data_.hasDefaultWindowing_)
    {
      return data_.defaultWindowingCenter_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  float DicomInstanceParameters::GetDefaultWindowingWidth() const
  {
    if (data_.hasDefaultWindowing_)
    {
      return data_.defaultWindowingWidth_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  TextureBaseSceneLayer* DicomInstanceParameters::CreateTexture(const Orthanc::ImageAccessor& pixelData) const
  {
    assert(sizeof(float) == 4);

    Orthanc::PixelFormat sourceFormat = pixelData.GetFormat();

    if (sourceFormat != GetExpectedPixelFormat())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    if (sourceFormat == Orthanc::PixelFormat_RGB24)
    {
      // This is the case of a color image. No conversion has to be done.
      return new ColorTextureSceneLayer(pixelData);
    }
    else
    {
      if (sourceFormat != Orthanc::PixelFormat_Grayscale16 &&
          sourceFormat != Orthanc::PixelFormat_Grayscale32 &&
          sourceFormat != Orthanc::PixelFormat_SignedGrayscale16)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
      }

      std::auto_ptr<FloatTextureSceneLayer> texture;
        
      {
        // This is the case of a grayscale frame. Convert it to Float32.
        std::auto_ptr<Orthanc::Image> converted(new Orthanc::Image(Orthanc::PixelFormat_Float32, 
                                                                   pixelData.GetWidth(), 
                                                                   pixelData.GetHeight(),
                                                                   false));
        Orthanc::ImageProcessing::Convert(*converted, pixelData);

        // Correct rescale slope/intercept if need be
        data_.ApplyRescale(*converted, (sourceFormat == Orthanc::PixelFormat_Grayscale32));

        texture.reset(new FloatTextureSceneLayer(*converted));
      }

      if (data_.hasDefaultWindowing_)
      {
        texture->SetCustomWindowing(data_.defaultWindowingCenter_,
                                    data_.defaultWindowingWidth_);
      }
        
      return texture.release();
    }
  }
}
