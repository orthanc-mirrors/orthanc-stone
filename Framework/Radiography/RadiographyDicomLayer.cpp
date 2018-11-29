/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2018 Osimis S.A., Belgium
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


#include "RadiographyDicomLayer.h"

#include "RadiographyScene.h"
#include "../Toolbox/DicomFrameConverter.h"

#include <Core/OrthancException.h>
#include <Core/Images/Image.h>
#include <Core/Images/ImageProcessing.h>
#include <Plugins/Samples/Common/DicomDatasetReader.h>

static OrthancPlugins::DicomTag  ConvertTag(const Orthanc::DicomTag& tag)
{
  return OrthancPlugins::DicomTag(tag.GetGroup(), tag.GetElement());
}

namespace OrthancStone
{

  void RadiographyDicomLayer::ApplyConverter()
  {
    if (source_.get() != NULL &&
        converter_.get() != NULL)
    {
      converted_.reset(converter_->ConvertFrame(*source_));
    }
  }


  void RadiographyDicomLayer::SetDicomTags(const OrthancPlugins::FullOrthancDataset& dataset)
  {
    converter_.reset(new DicomFrameConverter);
    converter_->ReadParameters(dataset);
    ApplyConverter();

    std::string tmp;
    Vector pixelSpacing;

    if (dataset.GetStringValue(tmp, ConvertTag(Orthanc::DICOM_TAG_PIXEL_SPACING)) &&
        LinearAlgebra::ParseVector(pixelSpacing, tmp) &&
        pixelSpacing.size() == 2)
    {
      SetPixelSpacing(pixelSpacing[0], pixelSpacing[1]);
    }

    //SetPan(-0.5 * GetPixelSpacingX(), -0.5 * GetPixelSpacingY());

    OrthancPlugins::DicomDatasetReader reader(dataset);

    unsigned int width, height;
    if (!reader.GetUnsignedIntegerValue(width, ConvertTag(Orthanc::DICOM_TAG_COLUMNS)) ||
        !reader.GetUnsignedIntegerValue(height, ConvertTag(Orthanc::DICOM_TAG_ROWS)))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
    else
    {
      SetSize(width, height);
    }
  }

  void RadiographyDicomLayer::SetSourceImage(Orthanc::ImageAccessor* image)   // Takes ownership
  {
    std::auto_ptr<Orthanc::ImageAccessor> raii(image);

    if (image == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    SetSize(image->GetWidth(), image->GetHeight());

    source_ = raii;
    ApplyConverter();
  }

  void RadiographyDicomLayer::Render(Orthanc::ImageAccessor& buffer,
                                     const AffineTransform2D& viewTransform,
                                     ImageInterpolation interpolation) const
  {
    if (converted_.get() != NULL)
    {
      if (converted_->GetFormat() != Orthanc::PixelFormat_Float32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      unsigned int cropX, cropY, cropWidth, cropHeight;
      GetCrop(cropX, cropY, cropWidth, cropHeight);

      AffineTransform2D t = AffineTransform2D::Combine(
            viewTransform, GetTransform(),
            AffineTransform2D::CreateOffset(cropX, cropY));

      Orthanc::ImageAccessor cropped;
      converted_->GetRegion(cropped, cropX, cropY, cropWidth, cropHeight);

      t.Apply(buffer, cropped, interpolation, false);
    }
  }


  bool RadiographyDicomLayer::GetDefaultWindowing(float& center,
                                                  float& width) const
  {
    if (converter_.get() != NULL &&
        converter_->HasDefaultWindow())
    {
      center = static_cast<float>(converter_->GetDefaultWindowCenter());
      width = static_cast<float>(converter_->GetDefaultWindowWidth());
      return true;
    }
    else
    {
      return false;
    }
  }


  bool RadiographyDicomLayer::GetRange(float& minValue,
                                       float& maxValue) const
  {
    if (converted_.get() != NULL)
    {
      if (converted_->GetFormat() != Orthanc::PixelFormat_Float32)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      Orthanc::ImageProcessing::GetMinMaxFloatValue(minValue, maxValue, *converted_);
      return true;
    }
    else
    {
      return false;
    }
  }

}
