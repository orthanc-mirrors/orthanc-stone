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


#pragma once

#include "../Volumes/ImageBuffer3D.h"
#include "GeometryToolbox.h"

#include <Images/ImageTraits.h>

#include <boost/noncopyable.hpp>
#include <cmath>

namespace OrthancStone
{
  namespace Internals
  {
    /*
    WARNING : the slice order is different between this class and ImageBuffer3D

    See the comment above ImageBuffer3D declaration.

    The slices are supposed to be stored in INCREASING z-order in this class!
    */
    class SubvoxelReaderBase : public boost::noncopyable
    {
    private:
      const ImageBuffer3D& source_;
      unsigned int         width_;
      unsigned int         height_;
      unsigned int         depth_;

    public:
      explicit SubvoxelReaderBase(const ImageBuffer3D& source) :
        source_(source),
        width_(source.GetWidth()),
        height_(source.GetHeight()),
        depth_(source.GetDepth())
      {
      }

      ORTHANC_FORCE_INLINE
      const Orthanc::ImageAccessor& GetSource() const
      {
        return source_.GetInternalImage();
      }
      
      ORTHANC_FORCE_INLINE
      unsigned int GetWidth() const
      {
        return width_;
      }
      
      ORTHANC_FORCE_INLINE
      unsigned int GetHeight() const
      {
        return height_;
      }
      
      ORTHANC_FORCE_INLINE
      unsigned int GetDepth() const
      {
        return depth_;
      }
      
      ORTHANC_FORCE_INLINE
      unsigned int ComputeRow(unsigned int y,
                              unsigned int z) const
      {
        return z * height_ + y;
      }
    };
  }

    
  /*
  WARNING : the slice order is different between this class and ImageBuffer3D

  See the comment above ImageBuffer3D declaration.

  The slices are supposed to be stored in INCREASING z-order in this class!
  */
  template <Orthanc::PixelFormat Format,
            ImageInterpolation Interpolation>
  class SubvoxelReader;

    
  /*
  WARNING : the slice order is different between this class and ImageBuffer3D

  See the comment above ImageBuffer3D declaration.

  The slices are supposed to be stored in INCREASING z-order in this class!
  */
  template <Orthanc::PixelFormat Format>
  class SubvoxelReader<Format, ImageInterpolation_Nearest> : 
    public Internals::SubvoxelReaderBase
  {
  public:
    typedef Orthanc::PixelTraits<Format>  Traits;
    typedef typename Traits::PixelType    PixelType;

    explicit SubvoxelReader(const ImageBuffer3D& source) :
      SubvoxelReaderBase(source)
    {
    }

    inline bool GetValue(PixelType& target,
                         float x,
                         float y,
                         float z) const;
    
    inline bool GetFloatValue(float& target,
                              float x,
                              float y,
                              float z) const;
  };
    
    
  /*
  WARNING : the slice order is different between this class and ImageBuffer3D

  See the comment above ImageBuffer3D declaration.

  The slices are supposed to be stored in INCREASING z-order in this class!
  */
  template <Orthanc::PixelFormat Format>
  class SubvoxelReader<Format, ImageInterpolation_Bilinear> : 
    public Internals::SubvoxelReaderBase
  {
  public:
    typedef Orthanc::PixelTraits<Format>  Traits;
    typedef typename Traits::PixelType    PixelType;

    explicit SubvoxelReader(const ImageBuffer3D& source) :
      SubvoxelReaderBase(source)
    {
    }

    inline bool Sample(float& f00,
                       float& f01,
                       float& f10,
                       float& f11,
                       unsigned int ux,
                       unsigned int uy,
                       unsigned int uz) const;

    inline bool GetValue(PixelType& target,
                         float x,
                         float y,
                         float z) const;

    inline bool GetFloatValue(float& target,
                              float x,
                              float y,
                              float z) const;
  };
    

  /*
  WARNING : the slice order is different between this class and ImageBuffer3D

  See the comment above ImageBuffer3D declaration.

  The slices are supposed to be stored in INCREASING z-order in this class!
  */
  template <Orthanc::PixelFormat Format>
  class SubvoxelReader<Format, ImageInterpolation_Trilinear> : 
    public Internals::SubvoxelReaderBase
  {
  private:
    SubvoxelReader<Format, ImageInterpolation_Bilinear>   bilinear_;

  public:
    typedef Orthanc::PixelTraits<Format>  Traits;
    typedef typename Traits::PixelType    PixelType;

    explicit SubvoxelReader(const ImageBuffer3D& source) :
      SubvoxelReaderBase(source),
      bilinear_(source)
    {
    }

    inline bool GetValue(PixelType& target,
                         float x,
                         float y,
                         float z) const;

    inline bool GetFloatValue(float& target,
                              float x,
                              float y,
                              float z) const;
  };


  /*
  See important comment above
  */

  template <Orthanc::PixelFormat Format>
  bool SubvoxelReader<Format, ImageInterpolation_Nearest>::GetValue(PixelType& target,
                                                                    float x,
                                                                    float y,
                                                                    float z) const
  {
    if (x < 0 ||
        y < 0 ||
        z < 0)
    {
      return false;
    }
    else
    {
      unsigned int ux = static_cast<unsigned int>(std::floor(x));
      unsigned int uy = static_cast<unsigned int>(std::floor(y));
      unsigned int uz = static_cast<unsigned int>(std::floor(z));

      if (ux < GetWidth() &&
          uy < GetHeight() &&
          uz < GetDepth())
      {
        Orthanc::ImageTraits<Format>::GetPixel(target, GetSource(), ux, ComputeRow(uy, uz));
        return true;
      }
      else
      {
        return false;
      }
    }
  }


  template <Orthanc::PixelFormat Format>
  bool SubvoxelReader<Format, ImageInterpolation_Nearest>::GetFloatValue(float& target,
                                                                         float x,
                                                                         float y,
                                                                         float z) const
  {
    PixelType value;
    
    if (GetValue(value, x, y, z))
    {
      target = Traits::PixelToFloat(value);
      return true;
    }
    else
    {
      return false;
    }
  }


  /*
  See important comment above
  */

  template <Orthanc::PixelFormat Format>
  bool SubvoxelReader<Format, ImageInterpolation_Bilinear>::Sample(float& f00,
                                                                   float& f01,
                                                                   float& f10,
                                                                   float& f11,
                                                                   unsigned int ux,
                                                                   unsigned int uy,
                                                                   unsigned int uz) const
  {
    if (ux < GetWidth() &&
        uy < GetHeight() &&
        uz < GetDepth())
    {
      f00 = Orthanc::ImageTraits<Format>::GetFloatPixel(GetSource(), ux, ComputeRow(uy, uz));
    }
    else
    {
      // Pixel is out of the volume
      return false;
    }

    if (ux + 1 < GetWidth())
    {
      f01 = Orthanc::ImageTraits<Format>::GetFloatPixel(GetSource(), ux + 1, ComputeRow(uy, uz));
    }
    else
    {
      f01 = f00;
    }

    if (uy + 1 < GetHeight())
    {
      f10 = Orthanc::ImageTraits<Format>::GetFloatPixel(GetSource(), ux, ComputeRow(uy + 1, uz));
    }
    else
    {
      f10 = f00;
    }

    if (ux + 1 < GetWidth() &&
        uy + 1 < GetHeight())
    {
      f11 = Orthanc::ImageTraits<Format>::GetFloatPixel(GetSource(), ux + 1, ComputeRow(uy + 1, uz));
    }
    else
    {
      f11 = f00;
    }

    return true;
  }


  /*
  See important comment above
  */

  template <Orthanc::PixelFormat Format>
  bool SubvoxelReader<Format, ImageInterpolation_Bilinear>::GetFloatValue(float& target,
                                                                          float x,
                                                                          float y,
                                                                          float z) const
  {
    x -= 0.5f;
    y -= 0.5f;
        
    if (x < 0 ||
        y < 0 ||
        z < 0)
    {
      return false;
    }
    else
    {
      unsigned int ux = static_cast<unsigned int>(std::floor(x));
      unsigned int uy = static_cast<unsigned int>(std::floor(y));
      unsigned int uz = static_cast<unsigned int>(std::floor(z));

      float f00, f01, f10, f11;
      if (Sample(f00, f01, f10, f11, ux, uy, uz))
      {
        float ax = x - static_cast<float>(ux);
        float ay = y - static_cast<float>(uy);

        target = GeometryToolbox::ComputeBilinearInterpolationUnitSquare(ax, ay, f00, f01, f10, f11);
        return true;
      }
      else
      {
        return false;
      }
    }
  }


  /*
  See important comment above
  */

  template <Orthanc::PixelFormat Format>
  bool SubvoxelReader<Format, ImageInterpolation_Bilinear>::GetValue(PixelType& target,
                                                                     float x,
                                                                     float y,
                                                                     float z) const
  {
    float value;

    if (GetFloatValue(value, x, y, z))
    {
      Traits::FloatToPixel(target, value);
      return true;
    }
    else
    {
      return false;
    }
  }



  template <Orthanc::PixelFormat Format>
  bool SubvoxelReader<Format, ImageInterpolation_Trilinear>::GetFloatValue(float& target,
                                                                           float x,
                                                                           float y,
                                                                           float z) const
  {
    x -= 0.5f;
    y -= 0.5f;
    z -= 0.5f;
        
    if (x < 0 ||
        y < 0 ||
        z < 0)
    {
      return false;
    }
    else
    {
      unsigned int ux = static_cast<unsigned int>(std::floor(x));
      unsigned int uy = static_cast<unsigned int>(std::floor(y));
      unsigned int uz = static_cast<unsigned int>(std::floor(z));

      float f000, f001, f010, f011;
      if (bilinear_.Sample(f000, f001, f010, f011, ux, uy, uz))
      {
        const float ax = x - static_cast<float>(ux);
        const float ay = y - static_cast<float>(uy);

        float f100, f101, f110, f111;

        if (bilinear_.Sample(f100, f101, f110, f111, ux, uy, uz + 1))
        {
          const float az = z - static_cast<float>(uz);
          target = GeometryToolbox::ComputeTrilinearInterpolationUnitSquare
            (ax, ay, az, f000, f001, f010, f011, f100, f101, f110, f111);
        }
        else
        {
          target = GeometryToolbox::ComputeBilinearInterpolationUnitSquare
            (ax, ay, f000, f001, f010, f011);
        }

        return true;
      }
      else
      {
        return false;
      }
    }
  }


  /*
  See important comment above
  */


  template <Orthanc::PixelFormat Format>
  bool SubvoxelReader<Format, ImageInterpolation_Trilinear>::GetValue(PixelType& target,
                                                                      float x,
                                                                      float y,
                                                                      float z) const
  {
    float value;

    if (GetFloatValue(value, x, y, z))
    {
      Traits::FloatToPixel(target, value);
      return true;
    }
    else
    {
      return false;
    }
  }
}
