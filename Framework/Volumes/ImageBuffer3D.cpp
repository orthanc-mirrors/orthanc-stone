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


#include "ImageBuffer3D.h"

#include "../../Resources/Orthanc/Core/Images/ImageProcessing.h"
#include "../../Resources/Orthanc/Core/Logging.h"
#include "../../Resources/Orthanc/Core/OrthancException.h"

#include <string.h>

namespace OrthancStone
{
  Orthanc::ImageAccessor ImageBuffer3D::GetAxialSliceAccessor(unsigned int slice,
                                                              bool readOnly)
  {
    if (slice >= depth_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    Orthanc::ImageAccessor accessor;

    if (readOnly)
    {
      accessor.AssignReadOnly(format_, width_, height_, image_.GetPitch(),
                              image_.GetConstRow(height_ * (depth_ - 1 - slice)));
    }
    else
    {
      accessor.AssignWritable(format_, width_, height_, image_.GetPitch(),
                              image_.GetRow(height_ * (depth_ - 1 - slice)));
    }

    return accessor;
  }


  Orthanc::ImageAccessor ImageBuffer3D::GetCoronalSliceAccessor(unsigned int slice,
                                                                bool readOnly)
  {
    if (slice >= height_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    Orthanc::ImageAccessor accessor;

    if (readOnly)
    {
      accessor.AssignReadOnly(format_, width_, depth_, image_.GetPitch() * height_,
                              image_.GetConstRow(slice));
    }
    else
    {
      accessor.AssignWritable(format_, width_, depth_, image_.GetPitch() * height_,
                              image_.GetRow(slice));
    }

    return accessor;
  }


  Orthanc::Image*  ImageBuffer3D::ExtractSagittalSlice(unsigned int slice) const
  {
    if (slice >= width_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    std::auto_ptr<Orthanc::Image> result(new Orthanc::Image(format_, height_, depth_, false));

    unsigned int bytesPerPixel = Orthanc::GetBytesPerPixel(format_);

    for (unsigned int z = 0; z < depth_; z++)
    {
      //uint8_t* target = reinterpret_cast<uint8_t*>(result->GetRow(depth_ - 1 - z));
      uint8_t* target = reinterpret_cast<uint8_t*>(result->GetRow(z));

      for (unsigned int y = 0; y < height_; y++)
      {
        const void* source = (reinterpret_cast<const uint8_t*>(image_.GetConstRow(y + z * height_)) + 
                              bytesPerPixel * slice);

        memcpy(target, source, bytesPerPixel);
        target += bytesPerPixel;
      }
    }

    return result.release();
  }


  ImageBuffer3D::ImageBuffer3D(Orthanc::PixelFormat format,
                               unsigned int width,
                               unsigned int height,
                               unsigned int depth) :
    image_(format, width, height * depth, false),
    format_(format),
    width_(width),
    height_(height),
    depth_(depth)
  {
    GeometryToolbox::AssignVector(voxelDimensions_, 1, 1, 1);

    LOG(INFO) << "Created an image of "
              << (GetEstimatedMemorySize() / (1024ll * 1024ll)) << "MB";
  }


  void ImageBuffer3D::Clear()
  {
    memset(image_.GetBuffer(), 0, image_.GetHeight() * image_.GetPitch());
  }


  void ImageBuffer3D::SetAxialGeometry(const CoordinateSystem3D& geometry)
  {
    axialGeometry_ = geometry;
  }


  void ImageBuffer3D::SetVoxelDimensions(double x,
                                         double y,
                                         double z)
  {
    if (x <= 0 ||
        y <= 0 ||
        z <= 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    {
      GeometryToolbox::AssignVector(voxelDimensions_, x, y, z);
    }
  }


  Vector ImageBuffer3D::GetVoxelDimensions(VolumeProjection projection)
  {
    Vector result;
    switch (projection)
    {
      case VolumeProjection_Axial:
        result = voxelDimensions_;
        break;

      case VolumeProjection_Coronal:
        GeometryToolbox::AssignVector(result, voxelDimensions_[0], voxelDimensions_[2], voxelDimensions_[1]);
        break;

      case VolumeProjection_Sagittal:
        GeometryToolbox::AssignVector(result, voxelDimensions_[1], voxelDimensions_[2], voxelDimensions_[0]);
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }

    return result;
  }


  void ImageBuffer3D::GetSliceSize(unsigned int& width,
                                   unsigned int& height,
                                   VolumeProjection projection)
  {
    switch (projection)
    {
      case VolumeProjection_Axial:
        width = width_;
        height = height_;
        break;

      case VolumeProjection_Coronal:
        width = width_;
        height = depth_;
        break;

      case VolumeProjection_Sagittal:
        width = height_;
        height = depth_;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  ParallelSlices* ImageBuffer3D::GetGeometry(VolumeProjection projection)
  {
    std::auto_ptr<ParallelSlices> result(new ParallelSlices);

    switch (projection)
    {
      case VolumeProjection_Axial:
        for (unsigned int z = 0; z < depth_; z++)
        {
          Vector origin = axialGeometry_.GetOrigin();
          origin += static_cast<double>(z) * voxelDimensions_[2] * axialGeometry_.GetNormal();

          result->AddSlice(origin, 
                           axialGeometry_.GetAxisX(), 
                           axialGeometry_.GetAxisY());
        }
        break;

      case VolumeProjection_Coronal:
        for (unsigned int y = 0; y < height_; y++)
        {
          Vector origin = axialGeometry_.GetOrigin();
          origin += static_cast<double>(y) * voxelDimensions_[1] * axialGeometry_.GetAxisY();
          origin += static_cast<double>(depth_ - 1) * voxelDimensions_[2] * axialGeometry_.GetNormal();

          result->AddSlice(origin, 
                           axialGeometry_.GetAxisX(), 
                           -axialGeometry_.GetNormal());
        }
        break;

      case VolumeProjection_Sagittal:
        for (unsigned int x = 0; x < width_; x++)
        {
          Vector origin = axialGeometry_.GetOrigin();
          origin += static_cast<double>(x) * voxelDimensions_[0] * axialGeometry_.GetAxisX();
          origin += static_cast<double>(depth_ - 1) * voxelDimensions_[2] * axialGeometry_.GetNormal();

          result->AddSlice(origin, 
                           axialGeometry_.GetAxisY(), 
                           -axialGeometry_.GetNormal());
        }
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);          
    }

    return result.release();
  }
    

  uint64_t ImageBuffer3D::GetEstimatedMemorySize() const
  {
    return image_.GetPitch() * image_.GetHeight() * Orthanc::GetBytesPerPixel(format_);
  }


  ImageBuffer3D::SliceReader::SliceReader(ImageBuffer3D& that,
                                          VolumeProjection projection,
                                          unsigned int slice)
  {
    switch (projection)
    {
      case VolumeProjection_Axial:
        accessor_ = that.GetAxialSliceAccessor(slice, true);
        break;

      case VolumeProjection_Coronal:
        accessor_ = that.GetCoronalSliceAccessor(slice, true);
        break;

      case VolumeProjection_Sagittal:
        sagittal_.reset(that.ExtractSagittalSlice(slice));
        accessor_ = *sagittal_;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);          
    }
  }


  void ImageBuffer3D::SliceWriter::Flush()
  {
    if (modified_)
    {
      if (sagittal_.get() != NULL)
      {
        // TODO
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);          
      }
    }
  }


  ImageBuffer3D::SliceWriter::SliceWriter(ImageBuffer3D& that,
                                          VolumeProjection projection,
                                          unsigned int slice) :
  modified_(false)
  {
    switch (projection)
    {
      case VolumeProjection_Axial:
        accessor_ = that.GetAxialSliceAccessor(slice, false);
        break;

      case VolumeProjection_Coronal:
        accessor_ = that.GetCoronalSliceAccessor(slice, false);
        break;

      case VolumeProjection_Sagittal:
        sagittal_.reset(that.ExtractSagittalSlice(slice));
        accessor_ = *sagittal_;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);          
    }
  }
}
