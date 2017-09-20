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
  }


  Orthanc::PixelFormat DicomFrameConverter::GetExpectedPixelFormat() const
  {
    // TODO Add more checks, e.g. on the number of bytes per value
    // (cf. DicomImageInformation.h in Orthanc)

    if (isColor_)
    {
      return Orthanc::PixelFormat_RGB24;
    }
    else if (isSigned_)
    {
      return Orthanc::PixelFormat_SignedGrayscale16;
    }
    else
    {
      return Orthanc::PixelFormat_Grayscale16;
    }
  }


  void DicomFrameConverter::ReadParameters(const OrthancPlugins::IDicomDataset& dicom)
  {
    SetDefaultParameters();

    Vector c, w;
    if (GeometryToolbox::ParseVector(c, dicom, OrthancPlugins::DICOM_TAG_WINDOW_CENTER) &&
        GeometryToolbox::ParseVector(w, dicom, OrthancPlugins::DICOM_TAG_WINDOW_WIDTH) &&
        c.size() > 0 && 
        w.size() > 0)
    {
      defaultWindowCenter_ = static_cast<float>(c[0]);
      defaultWindowWidth_ = static_cast<float>(w[0]);
    }

    OrthancPlugins::DicomDatasetReader reader(dicom);

    int tmp;
    if (!reader.GetIntegerValue(tmp, OrthancPlugins::DICOM_TAG_PIXEL_REPRESENTATION))
    {
      // Type 1 tag, must be present
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    isSigned_ = (tmp == 1);

    if (reader.GetFloatValue(rescaleIntercept_, OrthancPlugins::DICOM_TAG_RESCALE_INTERCEPT) &&
        reader.GetFloatValue(rescaleSlope_, OrthancPlugins::DICOM_TAG_RESCALE_SLOPE))
    {
      hasRescale_ = true;
    }

    // Type 1 tag, must be present
    std::string photometric = reader.GetMandatoryStringValue(OrthancPlugins::DICOM_TAG_PHOTOMETRIC_INTERPRETATION);
    photometric = Orthanc::Toolbox::StripSpaces(photometric);
    isColor_ = (photometric != "MONOCHROME1" &&
                photometric != "MONOCHROME2");
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
           sourceFormat == Orthanc::PixelFormat_SignedGrayscale16);

    // This is the case of a grayscale frame. Convert it to Float32.
    std::auto_ptr<Orthanc::Image> converted(new Orthanc::Image(Orthanc::PixelFormat_Float32, 
                                                               source->GetWidth(), 
                                                               source->GetHeight(),
                                                               false));
    Orthanc::ImageProcessing::Convert(*converted, *source);

    source.reset(NULL);  // We don't need the source frame anymore

    // Correct rescale slope/intercept if need be
    if (hasRescale_)
    {
      for (unsigned int y = 0; y < converted->GetHeight(); y++)
      {
        float* p = reinterpret_cast<float*>(converted->GetRow(y));
        for (unsigned int x = 0; x < converted->GetWidth(); x++, p++)
        {
          float value = *p;

          if (hasRescale_)
          {
            value = value * rescaleSlope_ + rescaleIntercept_;
          }
            
          *p = value;
        }
      }
    }
      
    source = converted;
  }
}
