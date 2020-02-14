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

#include "ImageToolbox.h"

#include "../StoneException.h"

#include <Core/Images/ImageProcessing.h>
#include <Core/Images/PixelTraits.h>

#include <Core/Logging.h>
#include <Core/OrthancException.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#include <vector>

namespace OrthancStone
{
  namespace
  {
    using Orthanc::PixelTraits;
    using Orthanc::PixelFormat;
    using Orthanc::ImageAccessor;
    using Orthanc::PixelFormat;

    template<typename Orthanc::PixelFormat Format>
    class PixelBinner
    {
      // "PixelBinner requires an arithmetic (integer or floating-point) pixel format"
      typedef typename Orthanc::PixelTraits<Format>::PixelType PixelType;
      BOOST_STATIC_ASSERT(boost::is_arithmetic<PixelType>::value);
    
    public:
      PixelBinner(HistogramData& hd, double minValue, double maxValue)
        : hd_(hd)
        , minValue_(minValue)
        , maxValue_(maxValue)
        , division_(1.0 / hd_.binSize)
      {
        ORTHANC_ASSERT(hd_.bins.size() > 0);
        ORTHANC_ASSERT(maxValue > minValue);
      }

      ORTHANC_FORCE_INLINE void AddPixel(PixelType p)
      {
        if (p <= minValue_)
        {
          hd_.bins[0] += 1;
        }
        else if (p >= maxValue_)
        {
          hd_.bins.back() += 1;
        }
        else
        {
          double distanceFromMin = p - minValue_;
          size_t binIndex = static_cast<size_t>(
            std::floor(distanceFromMin * division_));
          if (binIndex >= hd_.bins.size())
            binIndex = hd_.bins.size() - 1;
          hd_.bins[binIndex] += 1;
        }
      }
    private:
      HistogramData&        hd_;
      double                minValue_;
      double                maxValue_;
      double                division_;
    };

    template<PixelFormat Format>
    struct Histogram
    {
      typedef typename PixelTraits<Format>::PixelType PixelType;

      static void Apply(const Orthanc::ImageAccessor& img, HistogramData& hd,
                        double minValue = 0, 
                        double maxValue = 0)
      {
        ORTHANC_ASSERT(Format == img.GetFormat(), 
                       "Internal error. Wrong template histogram type");
    
        const uint8_t*  buffer = reinterpret_cast<const uint8_t*>(
          img.GetConstBuffer());

        const size_t pitch = img.GetPitch();
        const size_t bytesPerPix = img.GetBytesPerPixel();

        const size_t height = img.GetHeight();
        const size_t width = img.GetHeight();

        if ((minValue == 0) && (maxValue == 0))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
          //ORTHANC_ASSERT(boost::is_integral<PixelType>::value, 
          //  "Min and max values must be supplied for float-based histogram");
          //
          //PixelTraits<Format>::SetMinValue(minValue);
          //PixelTraits<Format>::SetMaxValue(maxValue);
        }

        hd.minValue = minValue;

        // the following code is not really pretty but ensures 
        size_t numBins = static_cast<size_t>(
          std::ceil((maxValue - minValue) / hd.binSize));

        hd.bins.resize(numBins);
        std::fill(hd.bins.begin(), hd.bins.end(), 0);

        PixelBinner<Format> binner(hd, minValue, maxValue);
        for (uint32_t y = 0; y < height; ++y)
        {
          const PixelType* curPix = reinterpret_cast<const PixelType*>(
            img.GetConstRow(y));
          
          for (uint32_t x = 0; x < width; x++, curPix++)
          {
            binner.AddPixel(*curPix);
          }
        }
      }
    };


    template<PixelFormat Format>
    struct ComputeMinMax__
    {
      typedef typename PixelTraits<Format>::PixelType PixelType;

      static void Apply(const Orthanc::ImageAccessor& img,
                        PixelType& minValue, PixelType& maxValue)
      {
        ORTHANC_ASSERT(Format == img.GetFormat(), 
                       "Internal error. Wrong template histogram type");

        const uint8_t* buffer = reinterpret_cast<const uint8_t*>(
          img.GetConstBuffer());
        
        const size_t   pitch = img.GetPitch();
        const size_t   bytesPerPix = img.GetBytesPerPixel();

        const size_t height = img.GetHeight();
        const size_t width = img.GetHeight();

        if (height * width == 0)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
        }

        // min and max are crossed below. Think about it. This is OK :)
        PixelTraits<Format>::SetMaxValue(minValue);
        PixelTraits<Format>::SetMinValue(maxValue);

        for (uint32_t y = 0; y < height; ++y)
        {
          const PixelType* curPix = reinterpret_cast<const PixelType*>(
            img.GetConstRow(y));

          for (uint32_t x = 0; x < width; x++, curPix++)
          {
            if (*curPix <= minValue)
              minValue = *curPix;
            if (*curPix >= maxValue)
              maxValue = *curPix;
          }
        }
      }
    };

    template<PixelFormat Format>
    void ComputeMinMax_(const Orthanc::ImageAccessor& img, 
                        double& minValue, double& maxValue)
    {
      typedef typename PixelTraits<Format>::PixelType PixelType;
      PixelType minValuePix = PixelType();
      PixelType maxValuePix = PixelType();
      ComputeMinMax__<Format>::Apply(img, minValuePix, maxValuePix);
      minValue = static_cast<double>(minValuePix);
      maxValue = static_cast<double>(maxValuePix);
    }
    
    template<PixelFormat Format>
    void ComputeHistogram_(const Orthanc::ImageAccessor& img, HistogramData& hd)
    {
      typedef typename PixelTraits<Format>::PixelType PixelType;
      PixelType minValue = PixelType();
      PixelType maxValue = PixelType();
      ComputeMinMax__<Format>::Apply(img, minValue, maxValue);
      
      // make bins a little bigger to center integer pixel values
      Histogram<Format>::Apply(img, hd, 
                               static_cast<double>(minValue) - 0.5, 
                               static_cast<double>(maxValue) + 0.5);
    }
  }

  void ComputeHistogram(const Orthanc::ImageAccessor& img,
                        HistogramData& hd, double binSize)
  {
    using namespace Orthanc;

    hd.binSize = binSize;

    // dynamic/static bridge
    switch (img.GetFormat())
    {
    case PixelFormat_Grayscale8:
      ComputeHistogram_<PixelFormat_Grayscale8>       (img, hd);
      break;
    case PixelFormat_Grayscale16:
      ComputeHistogram_<PixelFormat_Grayscale16>      (img, hd);
      break;
    case PixelFormat_SignedGrayscale16:
      ComputeHistogram_<PixelFormat_SignedGrayscale16>(img, hd);
      break;
    case PixelFormat_Float32:
      ComputeHistogram_<PixelFormat_Float32>          (img, hd);
      break;
    case PixelFormat_Grayscale32:
      ComputeHistogram_<PixelFormat_Grayscale32>      (img, hd);
      break;
    case PixelFormat_Grayscale64:
      ComputeHistogram_<PixelFormat_Grayscale64>      (img, hd);
      break;
    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }
  }

  void ComputeMinMax(const Orthanc::ImageAccessor& img,
                     double& minValue, double& maxValue)
  {
    using namespace Orthanc;

    // dynamic/static bridge
    switch (img.GetFormat())
    {
    case PixelFormat_Grayscale8:
      ComputeMinMax_<PixelFormat_Grayscale8>       (img, minValue, maxValue);
      break;                                                
    case PixelFormat_Grayscale16:                           
      ComputeMinMax_<PixelFormat_Grayscale16>      (img, minValue, maxValue);
      break;                                                
    case PixelFormat_SignedGrayscale16:                     
      ComputeMinMax_<PixelFormat_SignedGrayscale16>(img, minValue, maxValue);
      break;                                                
    case PixelFormat_Float32:                               
      ComputeMinMax_<PixelFormat_Float32>          (img, minValue, maxValue);
      break;                                                
    case PixelFormat_Grayscale32:                           
      ComputeMinMax_<PixelFormat_Grayscale32>      (img, minValue, maxValue);
      break;                                                
    case PixelFormat_Grayscale64:                           
      ComputeMinMax_<PixelFormat_Grayscale64>      (img, minValue, maxValue);
      break;
    default:
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
    }

  }

  void DumpHistogramResult(std::string& s, const HistogramData& hd)
  {
    std::stringstream ss;
    ss << "Histogram:\n";
    ss << "==========\n";
    ss << "\n";
    ss << "minValue        : " << hd.minValue << "\n";
    ss << "binSize         : " << hd.binSize << "\n";
    ss << "bins.size()     : " << hd.bins.size() << "\n";
    ss << "bins            :\n";
    double curBinStart = hd.minValue;
    size_t pixCount = 0;
    for (size_t i = 0; i < hd.bins.size(); ++i)
    {
      ss << "index: " << i << " (from " << curBinStart << " to "
        << curBinStart + hd.binSize << ") : " << hd.bins[i] << " pixels\n";
      curBinStart += hd.binSize;
      pixCount += hd.bins[i];
    }
    ss << "total pix. count: " << pixCount << "\n";
    s = ss.str();
  }
}
