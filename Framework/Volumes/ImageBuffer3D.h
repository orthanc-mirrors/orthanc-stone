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


#pragma once

#include "../Enumerations.h"
#include "../Toolbox/SliceGeometry.h"
#include "../Toolbox/ParallelSlices.h"

#include "../../Resources/Orthanc/Core/Images/Image.h"

#include <boost/thread/shared_mutex.hpp>

#if defined(_WIN32)
#  include <boost/thread/win32/mutex.hpp>
#endif

namespace OrthancStone
{
  class ImageBuffer3D : public boost::noncopyable
  {
  private:
    typedef boost::shared_mutex          Mutex;
    typedef boost::unique_lock<Mutex>    WriteLock;
    typedef boost::shared_lock<Mutex>    ReadLock;

    Mutex                  mutex_;
    SliceGeometry          axialGeometry_;
    Vector                 voxelDimensions_;
    Orthanc::Image         image_;
    Orthanc::PixelFormat   format_;
    unsigned int           width_;
    unsigned int           height_;
    unsigned int           depth_;

    Orthanc::ImageAccessor GetAxialSliceAccessor(unsigned int slice,
                                                 bool readOnly);

    Orthanc::ImageAccessor GetCoronalSliceAccessor(unsigned int slice,
                                                   bool readOnly);

    Orthanc::Image*  ExtractSagittalSlice(unsigned int slice) const;

  public:
    ImageBuffer3D(Orthanc::PixelFormat format,
                  unsigned int width,
                  unsigned int height,
                  unsigned int depth);

    void Clear();

    // Set the geometry of the first axial slice (i.e. the one whose
    // depth == 0)
    void SetAxialGeometry(const SliceGeometry& geometry);

    void SetVoxelDimensions(double x,
                            double y,
                            double z);

    Vector GetVoxelDimensions(VolumeProjection projection);

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

    ParallelSlices* GetGeometry(VolumeProjection projection);
    

    class SliceReader : public boost::noncopyable
    {
    private:
      ReadLock                       lock_;
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
      WriteLock                      lock_;
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

      Orthanc::ImageAccessor& GetAccessor()
      {
        return accessor_;
      }
    };
  };
}
