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


#include "DicomFrameConverter.h"

#include "GeometryToolbox.h"

#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Core/OrthancException.h>
#include <Core/Toolbox.h>

namespace OrthancStone
{
  void DicomFrameConverter::SetDefaultParameters()
  {
    isSigned_ = true;
    isColor_ = false;
    hasRescale_ = false;
    rescaleIntercept_ = 0;
    rescaleSlope_ = 1;
    defaultWindowCenter_ = 128;
    defaultWindowWidth_ = 256;
    expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale16;
  }


  void DicomFrameConverter::ReadParameters(const Orthanc::DicomMap& dicom)
  {
    SetDefaultParameters();

    Vector c, w;
    if (GeometryToolbox::ParseVector(c, dicom, Orthanc::DICOM_TAG_WINDOW_CENTER) &&
        GeometryToolbox::ParseVector(w, dicom, Orthanc::DICOM_TAG_WINDOW_WIDTH) &&
        c.size() > 0 && 
        w.size() > 0)
    {
      defaultWindowCenter_ = static_cast<float>(c[0]);
      defaultWindowWidth_ = static_cast<float>(w[0]);
    }

    int32_t tmp;
    if (!dicom.ParseInteger32(tmp, Orthanc::DICOM_TAG_PIXEL_REPRESENTATION))
    {
      // Type 1 tag, must be present
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    isSigned_ = (tmp == 1);

    double doseGridScaling;
    bool isRTDose = false;
    
    if (dicom.ParseDouble(rescaleIntercept_, Orthanc::DICOM_TAG_RESCALE_INTERCEPT) &&
        dicom.ParseDouble(rescaleSlope_, Orthanc::DICOM_TAG_RESCALE_SLOPE))
    {
      hasRescale_ = true;
    }
    else if (dicom.ParseDouble(doseGridScaling, Orthanc::DICOM_TAG_DOSE_GRID_SCALING))
    {
      // This is for RT-DOSE
      hasRescale_ = true;
      isRTDose = true;
      rescaleIntercept_ = 0;
      rescaleSlope_ = doseGridScaling;

      if (!dicom.ParseInteger32(tmp, Orthanc::DICOM_TAG_BITS_STORED))
      {
        // Type 1 tag, must be present
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      switch (tmp)
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

    std::string photometric;
    if (dicom.CopyToString(photometric, Orthanc::DICOM_TAG_PHOTOMETRIC_INTERPRETATION, false))
    {
      photometric = Orthanc::Toolbox::StripSpaces(photometric);
    }
    else
    {
      // Type 1 tag, must be present
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
    
    isColor_ = (photometric != "MONOCHROME1" &&
                photometric != "MONOCHROME2");

    // TODO Add more checks, e.g. on the number of bytes per value
    // (cf. DicomImageInformation.h in Orthanc)

    if (!isRTDose)
    {
      if (isColor_)
      {
        expectedPixelFormat_ = Orthanc::PixelFormat_RGB24;
      }
      else if (isSigned_)
      {
        expectedPixelFormat_ = Orthanc::PixelFormat_SignedGrayscale16;
      }
      else
      {
        expectedPixelFormat_ = Orthanc::PixelFormat_Grayscale16;
      }
    }
  }


  void DicomFrameConverter::ConvertFrame(std::auto_ptr<Orthanc::ImageAccessor>& source) const
  {
    assert(sizeof(float) == 4);

    if (source.get() == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    Orthanc::PixelFormat sourceFormat = source->GetFormat();

    if (sourceFormat != GetExpectedPixelFormat())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

    if (sourceFormat == Orthanc::PixelFormat_RGB24)
    {
      // No conversion has to be done
      return;
    }

    assert(sourceFormat == Orthanc::PixelFormat_Grayscale16 ||
           sourceFormat == Orthanc::PixelFormat_Grayscale32 ||
           sourceFormat == Orthanc::PixelFormat_SignedGrayscale16);

    // This is the case of a grayscale frame. Convert it to Float32.
    std::auto_ptr<Orthanc::Image> converted(new Orthanc::Image(Orthanc::PixelFormat_Float32, 
                                                               source->GetWidth(), 
                                                               source->GetHeight(),
                                                               false));
    Orthanc::ImageProcessing::Convert(*converted, *source);

    source.reset(NULL);  // We don't need the source frame anymore

    // Correct rescale slope/intercept if need be
    ApplyRescale(*converted, sourceFormat != Orthanc::PixelFormat_Grayscale32);
      
    source = converted;
  }


  void DicomFrameConverter::ApplyRescale(Orthanc::ImageAccessor& image,
                                         bool useDouble) const
  {
    if (image.GetFormat() != Orthanc::PixelFormat_Float32)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }
    
    if (hasRescale_)
    {
      for (unsigned int y = 0; y < image.GetHeight(); y++)
      {
        float* p = reinterpret_cast<float*>(image.GetRow(y));

        if (useDouble)
        {
          // Slower, accurate implementation using double
          for (unsigned int x = 0; x < image.GetWidth(); x++, p++)
          {
            double value = static_cast<double>(*p);
            *p = static_cast<float>(value * rescaleSlope_ + rescaleIntercept_);
          }
        }
        else
        {
          // Fast, approximate implementation using float
          for (unsigned int x = 0; x < image.GetWidth(); x++, p++)
          {
            *p = (*p) * static_cast<float>(rescaleSlope_) + static_cast<float>(rescaleIntercept_);
          }
        }
      }
    }
  }

  
  double DicomFrameConverter::Apply(double x) const
  {
    return x * rescaleSlope_ + rescaleIntercept_;
  }

}
