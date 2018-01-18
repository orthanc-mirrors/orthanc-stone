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


#pragma once

#include "../Enumerations.h"
#include "../Layers/RenderStyle.h"
#include "../Toolbox/CoordinateSystem3D.h"
#include "../Toolbox/DicomFrameConverter.h"
#include "../Toolbox/ParallelSlices.h"

#include <Core/Images/Image.h>

namespace OrthancStone
{
  class ImageBuffer3D : public boost::noncopyable
  {
  private:
    CoordinateSystem3D     axialGeometry_;
    Vector                 voxelDimensions_;
    Orthanc::Image         image_;
    Orthanc::PixelFormat   format_;
    unsigned int           width_;
    unsigned int           height_;
    unsigned int           depth_;
    bool                   computeRange_;
    bool                   hasRange_;
    float                  minValue_;
    float                  maxValue_;

    void ExtendImageRange(const Orthanc::ImageAccessor& slice);

    Orthanc::ImageAccessor GetAxialSliceAccessor(unsigned int slice,
                                                 bool readOnly);

    Orthanc::ImageAccessor GetCoronalSliceAccessor(unsigned int slice,
                                                   bool readOnly);

    Orthanc::Image*  ExtractSagittalSlice(unsigned int slice) const;

  public:
    ImageBuffer3D(Orthanc::PixelFormat format,
                  unsigned int width,
                  unsigned int height,
                  unsigned int depth,
                  bool computeRange);

    void Clear();

    // Set the geometry of the first axial slice (i.e. the one whose
    // depth == 0)
    void SetAxialGeometry(const CoordinateSystem3D& geometry);

    const CoordinateSystem3D& GetAxialGeometry() const
    {
      return axialGeometry_;
    }

    void SetVoxelDimensions(double x,
                            double y,
                            double z);

    Vector GetVoxelDimensions(VolumeProjection projection) const;

    void GetSliceSize(unsigned int& width,
                      unsigned int& height,
                      VolumeProjection projection);

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

    ParallelSlices* GetGeometry(VolumeProjection projection) const;
    
    uint64_t GetEstimatedMemorySize() const;

    bool GetRange(float& minValue,
                  float& maxValue) const;

    bool FitWindowingToRange(RenderStyle& style,
                             const DicomFrameConverter& converter) const;

    uint8_t GetPixelGrayscale8(unsigned int x,
                               unsigned int y,
                               unsigned int z) const;

    uint16_t GetPixelGrayscale16(unsigned int x,
                                 unsigned int y,
                                 unsigned int z) const;


    class SliceReader : public boost::noncopyable
    {
    private:
      Orthanc::ImageAccessor         accessor_;
      std::auto_ptr<Orthanc::Image>  sagittal_;  // Unused for axial and coronal

    public:
      SliceReader(ImageBuffer3D& that,
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
      std::auto_ptr<Orthanc::Image>  sagittal_;  // Unused for axial and coronal

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
