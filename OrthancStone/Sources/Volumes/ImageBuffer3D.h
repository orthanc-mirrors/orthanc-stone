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


#pragma once

#include "../StoneEnumerations.h"
#include "../Toolbox/LinearAlgebra.h"

#include <Compatibility.h>
#include <Images/Image.h>

namespace OrthancStone
{
  /*

  This classes stores volume images sliced across the Z axis, vertically, in the decreasing Z order :

  +---------------+
  |               |
  |   SLICE N-1   |
  |               |
  +---------------+
  |               |
  |   SLICE N-2   |
  |               |
  +---------------+
  |               |
  |   SLICE N-3   |
  |               |
  .               .
  ......     ......
  .               .
  |               |
  |   SLICE   2   |
  |               |
  +---------------+
  |               |
  |   SLICE   1   |
  |               |
  +---------------+
  |               |
  |   SLICE   0   |
  |               |
  +---------------+

  As you can see, if the 3d image has size width, height, depth, the 2d image has :
  - 2d width  = 3d width
  - 2d height = 3d height * 3d depth

  */

  class ImageBuffer3D : public boost::noncopyable
  {
  private:
    Orthanc::Image         image_;
    Orthanc::PixelFormat   format_;
    unsigned int           width_;
    unsigned int           height_;
    unsigned int           depth_;
    bool                   computeRange_;
    bool                   hasRange_;
    float                  minValue_;
    float                  maxValue_;
    Matrix                 transform_;
    Matrix                 transformInverse_;

    void ExtendImageRange(const Orthanc::ImageAccessor& slice);

    void GetAxialSliceAccessor(Orthanc::ImageAccessor& target,
                               unsigned int slice,
                               bool readOnly) const;
    
    void GetCoronalSliceAccessor(Orthanc::ImageAccessor& target,
                                 unsigned int slice,
                                 bool readOnly) const;

    Orthanc::Image*  ExtractSagittalSlice(unsigned int slice) const;

    template <typename T>
    T GetPixelUnchecked(unsigned int x,
                        unsigned int y,
                        unsigned int z) const
    {
      const uint8_t* buffer = reinterpret_cast<const uint8_t*>(image_.GetConstBuffer());
      const uint8_t* row = buffer + (y + height_ * (depth_ - 1 - z)) * image_.GetPitch();
      return reinterpret_cast<const T*>(row) [x];
    }

  public:
    ImageBuffer3D(Orthanc::PixelFormat format,
                  unsigned int width,
                  unsigned int height,
                  unsigned int depth,
                  bool computeRange);

    void Clear();

    const Orthanc::ImageAccessor& GetInternalImage() const
    {
      return image_;
    }

    unsigned int GetWidth() const
    {
      return width_;
    }

    unsigned int GetHeight() const
    {
      return height_;
    }

    unsigned int GetDepth() const
    {
      return depth_;
    }

    Orthanc::PixelFormat GetFormat() const
    {
      return format_;
    }

    unsigned int GetBytesPerPixel() const
    {
      return Orthanc::GetBytesPerPixel(format_);
    }

    uint64_t GetEstimatedMemorySize() const;

    bool GetRange(float& minValue,
                  float& maxValue) const;

    uint8_t GetVoxelGrayscale8Unchecked(unsigned int x,
                                        unsigned int y,
                                        unsigned int z) const
    {
      return GetPixelUnchecked<uint8_t>(x, y, z);
    }

    uint16_t GetVoxelGrayscale16Unchecked(unsigned int x,
                                          unsigned int y,
                                          unsigned int z) const
    {
      return GetPixelUnchecked<uint16_t>(x, y, z);
    }

    int16_t GetVoxelSignedGrayscale16Unchecked(unsigned int x,
                                               unsigned int y,
                                               unsigned int z) const
    {
      return GetPixelUnchecked<int16_t>(x, y, z);
    }

    uint8_t GetVoxelGrayscale8(unsigned int x,
                               unsigned int y,
                               unsigned int z) const;

    uint16_t GetVoxelGrayscale16(unsigned int x,
                                 unsigned int y,
                                 unsigned int z) const;

    
    class SliceReader : public boost::noncopyable
    {
    private:
      Orthanc::ImageAccessor         accessor_;
      std::unique_ptr<Orthanc::Image>  sagittal_;  // Unused for axial and coronal

    public:
      SliceReader(const ImageBuffer3D& that,
                  VolumeProjection projection,
                  unsigned int slice);

      const Orthanc::ImageAccessor& GetAccessor() const
      {
        return accessor_;
      }
    };


    class SliceWriter : public boost::noncopyable
    {
    private:
      ImageBuffer3D&                 that_;
      bool                           modified_;
      Orthanc::ImageAccessor         accessor_;
      std::unique_ptr<Orthanc::Image>  sagittal_;  // Unused for axial and coronal

      void Flush();

    public:
      SliceWriter(ImageBuffer3D& that,
                  VolumeProjection projection,
                  unsigned int slice);

      ~SliceWriter()
      {
        Flush();
      }

      const Orthanc::ImageAccessor& GetAccessor() const
      {
        return accessor_;
      }

      Orthanc::ImageAccessor& GetAccessor()
      {
        modified_ = true;
        return accessor_;
      }
    };
  };
}