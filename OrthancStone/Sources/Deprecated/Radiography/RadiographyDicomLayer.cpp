/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2020 Osimis S.A., Belgium
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
#include "../Deprecated/Toolbox/DicomFrameConverter.h"
#include "../Toolbox/ImageGeometry.h"

#include <OrthancException.h>
#include <Images/Image.h>
#include <Images/ImageProcessing.h>
#include <DicomDatasetReader.h>

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


  RadiographyDicomLayer::RadiographyDicomLayer(const RadiographyScene& scene) :
    RadiographyLayer(scene)
  {

  }

  void RadiographyDicomLayer::SetDicomTags(const OrthancPlugins::FullOrthancDataset& dataset)
  {
    converter_.reset(new Deprecated::DicomFrameConverter);
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

    if (dataset.GetStringValue(tmp, ConvertTag(Orthanc::DICOM_TAG_PHOTOMETRIC_INTERPRETATION)))
    {
      if (tmp == "MONOCHROME1")
      {
        SetPreferredPhotomotricDisplayMode(RadiographyPhotometricDisplayMode_Monochrome1);
      }
      else if (tmp == "MONOCHROME2")
      {
        SetPreferredPhotomotricDisplayMode(RadiographyPhotometricDisplayMode_Monochrome2);
      }
    }
  }

  void RadiographyDicomLayer::SetSourceImage(Orthanc::ImageAccessor* image)   // Takes ownership
  {
    std::unique_ptr<Orthanc::ImageAccessor> raii(image);

    if (image == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    SetSize(image->GetWidth(), image->GetHeight());

#if __cplusplus < 201103L
      source_.reset(raii.release());
#else
      source_ = std::move(raii);
#endif

    ApplyConverter();

    BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
  }

  void RadiographyDicomLayer::SetSourceImage(Orthanc::ImageAccessor* image, double newPixelSpacingX, double newPixelSpacingY, bool emitLayerEditedEvent)   // Takes ownership
  {
    std::unique_ptr<Orthanc::ImageAccessor> raii(image);

    if (image == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }

    SetSize(image->GetWidth(), image->GetHeight(), false);

#if __cplusplus < 201103L
    source_.reset(raii.release());
#else
    source_ = std::move(raii);
#endif

    ApplyConverter();

    SetPixelSpacing(newPixelSpacingX, newPixelSpacingY, false);

    if (emitLayerEditedEvent)
    {
      BroadcastMessage(RadiographyLayer::LayerEditedMessage(*this));
    }
  }


  void RadiographyDicomLayer::SetDicomFrameConverter(Deprecated::DicomFrameConverter* converter)
  {
    converter_.reset(converter);
  }

  void RadiographyDicomLayer::Render(Orthanc::ImageAccessor& buffer,
                                     const AffineTransform2D& viewTransform,
                                     ImageInterpolation interpolation,
                                     float windowCenter,
                                     float windowWidth,
                                     bool applyWindowing) const
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

      unsigned int x1, y1, x2, y2;
      if (!OrthancStone::GetProjectiveTransformExtent(x1, y1, x2, y2,
                                                      t.GetHomogeneousMatrix(),
                                                      cropped.GetWidth(),
                                                      cropped.GetHeight(),
                                                      buffer.GetWidth(),
                                                      buffer.GetHeight()))
      {
        return;  // layer is outside the buffer
      }

      t.Apply(buffer, cropped, interpolation, false);

      if (applyWindowing)
      {
        // apply windowing but stay in the range [0.0, 65535.0]
        float w0 = windowCenter - windowWidth / 2.0f;
        float w1 = windowCenter + windowWidth / 2.0f;

        if (windowWidth >= 0.001f)  // Avoid division by zero at (*)
        {
          float scaling = 1.0f / (w1 - w0) * 65535.0f;
          for (unsigned int y = y1; y <= y2; y++)
          {
            float* p = reinterpret_cast<float*>(buffer.GetRow(y)) + x1;

            for (unsigned int x = x1; x <= x2; x++, p++)
            {
              if (*p >= w1)
              {
                *p = 65535.0;
              }
              else if (*p <= w0)
              {
                *p = 0;
              }
              else
              {
                // https://en.wikipedia.org/wiki/Linear_interpolation
                *p = scaling * (*p - w0);  // (*)
              }
            }
          }
        }
      }

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