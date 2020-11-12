/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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


#include "DicomInstanceParameters.h"

#include "../Scene2D/ColorTextureSceneLayer.h"
#include "../Scene2D/FloatTextureSceneLayer.h"
#include "../Toolbox/GeometryToolbox.h"
#include "../Toolbox/ImageToolbox.h"

#include <Images/Image.h>
#include <Images/ImageProcessing.h>
#include <Logging.h>
#include <OrthancException.h>
#include <Toolbox.h>


namespace OrthancStone
{
  static void ExtractFrameOffsets(Vector& target,
                                  const Orthanc::DicomMap& dicom,
                                  unsigned int numberOfFrames)
  {
    // http://dicom.nema.org/medical/Dicom/2016a/output/chtml/part03/sect_C.8.8.3.2.html

    std::string increment;

    if (dicom.LookupStringValue(increment, Orthanc::DICOM_TAG_FRAME_INCREMENT_POINTER, false))
    {
      Orthanc::Toolbox::ToUpperCase(increment);

      // This is the "Grid Frame Offset Vector" tag (DICOM_TAG_GRID_FRAME_OFFSET_VECTOR)
      if (increment != "3004,000C")
      {
        LOG(WARNING) << "Bad value for the FrameIncrementPointer tags in a multiframe image";
        target.resize(0);
        return;
      }
    }

    if (!LinearAlgebra::ParseVector(target, dicom, Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR) ||
        target.size() != numberOfFrames)
    {
      LOG(INFO) << "The frame offset information is missing in a multiframe image";

      // DO NOT use ".clear()" here, as the "Vector" class doesn't behave like std::vector!
      target.resize(0);
    }
  }


  DicomInstanceParameters::Data::Data(const Orthanc::DicomMap& dicom)
  {
    if (!dicom.LookupStringValue(studyInstanceUid_, Orthanc::DICOM_TAG_STUDY_INSTANCE_UID, false) ||
        !dicom.LookupStringValue(seriesInstanceUid_, Orthanc::DICOM_TAG_SERIES_INSTANCE_UID, false) ||
        !dicom.LookupStringValue(sopInstanceUid_, Orthanc::DICOM_TAG_SOP_INSTANCE_UID, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
        
    std::string s;
    if (dicom.LookupStringValue(s, Orthanc::DICOM_TAG_SOP_CLASS_UID, false))
    {
      sopClassUid_ = StringToSopClassUid(s);
    }
    else
    {
      sopClassUid_ = SopClassUid_Other;
    }

    uint32_t n;
    if (dicom.ParseUnsignedInteger32(n, Orthanc::DICOM_TAG_NUMBER_OF_FRAMES))
    {
      numberOfFrames_ = n;
    }
    else
    {
      numberOfFrames_ = 1;
    }

    if (!dicom.HasTag(Orthanc::DICOM_TAG_COLUMNS) ||
        !dicom.GetValue(Orthanc::DICOM_TAG_COLUMNS).ParseFirstUnsignedInteger(width_))
    {
      width_ = 0;
    }    

    if (!dicom.HasTag(Orthanc::DICOM_TAG_ROWS) ||
        !dicom.GetValue(Orthanc::DICOM_TAG_ROWS).ParseFirstUnsignedInteger(height_))
    {
      height_ = 0;
     }

    if (!dicom.ParseDouble(sliceThickness_, Orthanc::DICOM_TAG_SLICE_THICKNESS))
    {
      sliceThickness_ = 100.0 * std::numeric_limits<double>::epsilon();
    }

    GeometryToolbox::GetPixelSpacing(pixelSpacingX_, pixelSpacingY_, dicom);

    std::string position, orientation;
    if (dicom.LookupStringValue(position, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT, false) &&
        dicom.LookupStringValue(orientation, Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT, false))
    {
      geometry_ = CoordinateSystem3D(position, orientation);
    }

    // Must be AFTER setting "numberOfFrames_"
    if (numberOfFrames_ > 1)
    {
      ExtractFrameOffsets(frameOffsets_, dicom, numberOfFrames_);
    }
    else
    {
      frameOffsets_.resize(0);
    }

    if (sopClassUid_ == SopClassUid_RTDose)
    {
      static const Orthanc::DicomTag DICOM_TAG_DOSE_UNITS(0x3004, 0x0002);

      if (!dicom.LookupStringValue(doseUnits_, DICOM_TAG_DOSE_UNITS, false))
      {
        LOG(ERROR) << "Tag DoseUnits (0x3004, 0x0002) is missing in " << sopInstanceUid_;
        doseUnits_.clear();
      }
    }

    if (dicom.ParseDouble(rescaleIntercept_, Orthanc::DICOM_TAG_RESCALE_INTERCEPT) &&
        dicom.ParseDouble(rescaleSlope_, Orthanc::DICOM_TAG_RESCALE_SLOPE))
    {
      if (sopClassUid_ == SopClassUid_RTDose)
      {
        LOG(INFO) << "DOSE HAS Rescale*: rescaleIntercept_ = " << rescaleIntercept_ << " rescaleSlope_ = " << rescaleSlope_;
        // WE SHOULD NOT TAKE THE RESCALE VALUE INTO ACCOUNT IN THE CASE OF DOSES
        hasRescale_ = false;
      }
      else
      {
        hasRescale_ = true;
      }
      
    }
    else
    {
      hasRescale_ = false;
    }

    if (dicom.ParseDouble(doseGridScaling_, Orthanc::DICOM_TAG_DOSE_GRID_SCALING))
    {
      if (sopClassUid_ == SopClassUid_RTDose)
      {
        LOG(INFO) << "DOSE HAS DoseGridScaling: doseGridScaling_ = " << doseGridScaling_;
      }
    }
    else
    {
      doseGridScaling_ = 1.0;
      if (sopClassUid_ == SopClassUid_RTDose)
      {
        LOG(ERROR) << "Tag DoseGridScaling (0x3004, 0x000e) is missing in " << sopInstanceUid_ << " doseGridScaling_ will be set to 1.0";
      }
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
      defaultWindowingCenter_ = 0;
      defaultWindowingWidth_  = 0;
    }

    // This computes the "IndexInSeries" metadata from Orthanc (check
    // out "Orthanc::ServerIndex::Store()")
    hasIndexInSeries_ = (
      dicom.ParseUnsignedInteger32(indexInSeries_, Orthanc::DICOM_TAG_INSTANCE_NUMBER) ||
      dicom.ParseUnsignedInteger32(indexInSeries_, Orthanc::DICOM_TAG_IMAGE_INDEX));

    if (!dicom.LookupStringValue(
          frameOfReferenceUid_, Orthanc::DICOM_TAG_FRAME_OF_REFERENCE_UID, false))
    {
      frameOfReferenceUid_.clear();
    }
  }


  const Orthanc::DicomImageInformation& DicomInstanceParameters::GetImageInformation() const
  {
    assert(tags_.get() != NULL);
    
    if (imageInformation_.get() == NULL)
    {
      const_cast<DicomInstanceParameters&>(*this).imageInformation_.
        reset(new Orthanc::DicomImageInformation(GetTags()));

      assert(imageInformation_->GetWidth() == GetWidth());
      assert(imageInformation_->GetHeight() == GetHeight());
      assert(imageInformation_->GetNumberOfFrames() == GetNumberOfFrames());
    }

    assert(imageInformation_.get() != NULL);
    return *imageInformation_;
  }
  

  CoordinateSystem3D  DicomInstanceParameters::GetFrameGeometry(unsigned int frame) const
  {
    if (frame >= data_.numberOfFrames_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else if (data_.frameOffsets_.empty())
    {
      return data_.geometry_;
    }
    else
    {
      assert(data_.frameOffsets_.size() == data_.numberOfFrames_);

      return CoordinateSystem3D(
        data_.geometry_.GetOrigin() + data_.frameOffsets_[frame] * data_.geometry_.GetNormal(),
        data_.geometry_.GetAxisX(),
        data_.geometry_.GetAxisY());
    }
  }


  bool DicomInstanceParameters::IsPlaneWithinSlice(unsigned int frame,
                                                   const CoordinateSystem3D& plane) const
  {
    if (frame >= data_.numberOfFrames_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    CoordinateSystem3D tmp = data_.geometry_;

    if (frame != 0)
    {
      tmp = GetFrameGeometry(frame);
    }

    double distance;

    return (CoordinateSystem3D::ComputeDistance(distance, tmp, plane) &&
            distance <= data_.sliceThickness_ / 2.0);
  }


  bool DicomInstanceParameters::IsColor() const
  {
    Orthanc::PhotometricInterpretation photometric =
      GetImageInformation().GetPhotometricInterpretation();
    
    return (photometric != Orthanc::PhotometricInterpretation_Monochrome1 &&
            photometric != Orthanc::PhotometricInterpretation_Monochrome2);
  }


  void DicomInstanceParameters::ApplyRescaleAndDoseScaling(Orthanc::ImageAccessor& image,
                                                           bool useDouble) const
  {
    if (image.GetFormat() != Orthanc::PixelFormat_Float32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    double factor = data_.doseGridScaling_;
    double offset = 0.0;

    if (data_.hasRescale_)
    {
      factor *= data_.rescaleSlope_;
      offset = data_.rescaleIntercept_;
    }

    if (!LinearAlgebra::IsNear(factor, 1) ||
        !LinearAlgebra::IsNear(offset, 0))
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
            *p = static_cast<float>(value * factor + offset);
          }
        }
        else
        {
          // Fast, approximate implementation using float
          for (unsigned int x = 0; x < width; x++, p++)
          {
            *p = (*p) * static_cast<float>(factor) + static_cast<float>(offset);
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
      LOG(ERROR) << "DicomInstanceParameters::GetRescaleIntercept(): !data_.hasRescale_";
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
      LOG(ERROR) << "DicomInstanceParameters::GetRescaleSlope(): !data_.hasRescale_";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  Orthanc::PixelFormat DicomInstanceParameters::GetExpectedPixelFormat() const
  {
    if (GetSopClassUid() == SopClassUid_RTDose)
    {
      switch (GetImageInformation().GetBitsStored())
      {
        case 16:
          return Orthanc::PixelFormat_Grayscale16;

        case 32:
          return Orthanc::PixelFormat_Grayscale32;

        default:
          return Orthanc::PixelFormat_Grayscale16;  // Rough guess
      } 
    }
    else if (IsColor())
    {
      return Orthanc::PixelFormat_RGB24;
    }
    else if (GetImageInformation().IsSigned())
    {
      return Orthanc::PixelFormat_SignedGrayscale16;
    }
    else
    {
      return Orthanc::PixelFormat_Grayscale16;  // Rough guess
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
      LOG(ERROR) << "DicomInstanceParameters::GetDefaultWindowingCenter(): no default windowing";
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
      LOG(ERROR) << "DicomInstanceParameters::GetDefaultWindowingWidth(): no default windowing";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  Orthanc::ImageAccessor* DicomInstanceParameters::ConvertToFloat(const Orthanc::ImageAccessor& pixelData) const
  {
    std::unique_ptr<Orthanc::Image> converted(new Orthanc::Image(Orthanc::PixelFormat_Float32, 
                                                                 pixelData.GetWidth(), 
                                                                 pixelData.GetHeight(),
                                                                 false));
    Orthanc::ImageProcessing::Convert(*converted, pixelData);

                                                   
    // Correct rescale slope/intercept if need be
    //ApplyRescaleAndDoseScaling(*converted, (pixelData.GetFormat() == Orthanc::PixelFormat_Grayscale32));
    ApplyRescaleAndDoseScaling(*converted, false);

    return converted.release();
  }


  TextureBaseSceneLayer* DicomInstanceParameters::CreateTexture
  (const Orthanc::ImageAccessor& pixelData) const
  {
    // {
    //   const Orthanc::ImageAccessor& source = pixelData;
    //   const void* sourceBuffer = source.GetConstBuffer();
    //   intptr_t sourceBufferInt = reinterpret_cast<intptr_t>(sourceBuffer);
    //   int sourceWidth = source.GetWidth();
    //   int sourceHeight = source.GetHeight();
    //   int sourcePitch = source.GetPitch();

    //   // TODO: turn error into trace below
    //   LOG(ERROR) << "ConvertGrayscaleToFloat | source:"
    //     << " W = " << sourceWidth << " H = " << sourceHeight
    //     << " P = " << sourcePitch << " B = " << sourceBufferInt
    //     << " B % 4 == " << sourceBufferInt % 4;
    // }

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
      // This is the case of a grayscale frame. Convert it to Float32.
      std::unique_ptr<FloatTextureSceneLayer> texture;

      if (pixelData.GetFormat() == Orthanc::PixelFormat_Float32)
      {
        texture.reset(new FloatTextureSceneLayer(pixelData));
      }
      else
      {
        std::unique_ptr<Orthanc::ImageAccessor> converted(ConvertToFloat(pixelData));
        texture.reset(new FloatTextureSceneLayer(*converted));
      }

      if (data_.hasDefaultWindowing_)
      {
        texture->SetCustomWindowing(data_.defaultWindowingCenter_,
                                    data_.defaultWindowingWidth_);
      }
      
      switch (GetImageInformation().GetPhotometricInterpretation())
      {
        case Orthanc::PhotometricInterpretation_Monochrome1:
          texture->SetInverted(true);
          break;
          
        case Orthanc::PhotometricInterpretation_Monochrome2:
          texture->SetInverted(false);
          break;

        default:
          break;
      }

      return texture.release();
    }
  }


  LookupTableTextureSceneLayer* DicomInstanceParameters::CreateLookupTableTexture
  (const Orthanc::ImageAccessor& pixelData) const
  {
    std::unique_ptr<FloatTextureSceneLayer> texture;

    if (pixelData.GetFormat() == Orthanc::PixelFormat_Float32)
    {
      return new LookupTableTextureSceneLayer(pixelData);
    }
    else
    {
      std::unique_ptr<Orthanc::ImageAccessor> converted(ConvertToFloat(pixelData));
      return new LookupTableTextureSceneLayer(*converted);
    }
  }

  
  unsigned int DicomInstanceParameters::GetIndexInSeries() const
  {
    if (data_.hasIndexInSeries_)
    {
      return data_.indexInSeries_;
    }
    else
    {
      LOG(ERROR) << "DicomInstanceParameters::GetIndexInSeries(): !data_.hasIndexInSeries_";
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  double DicomInstanceParameters::ApplyRescale(double value) const
  {
    double factor = data_.doseGridScaling_;
    double offset = 0.0;

    if (data_.hasRescale_)
    {
      factor *= data_.rescaleSlope_;
      offset = data_.rescaleIntercept_;
    }

    return (value * factor + offset);
  }


  bool DicomInstanceParameters::ComputeRegularSpacing(double& spacing) const
  {
    if (data_.frameOffsets_.size() == 0)  // Not a RT-DOSE
    {
      return false;
    }
    else if (data_.frameOffsets_.size() == 1)
    {
      spacing = 1;   // Edge case: RT-DOSE with one single frame
      return true;
    }
    else
    {
      assert(data_.frameOffsets_.size() == GetNumberOfFrames());
      
      spacing = std::abs(data_.frameOffsets_[1] - data_.frameOffsets_[0]);

      for (size_t i = 1; i + 1 < data_.frameOffsets_.size(); i++)
      {
        double s = data_.frameOffsets_[i + 1] - data_.frameOffsets_[i];
        if (!LinearAlgebra::IsNear(spacing, s, 0.001))
        {
          return false;
        }
      }
      
      return true;
    }
  }
}
