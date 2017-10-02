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

#include <Core/DicomFormat/DicomMap.h>
#include <Core/Images/ImageAccessor.h>

#include <memory>

namespace OrthancStone
{
  /**
   * This class is responsible for converting the pixel format of a
   * DICOM frame coming from Orthanc, into a pixel format that is
   * suitable for Stone, given the relevant DICOM tags:
   * - Color frames will stay in the RGB24 format.
   * - Grayscale frames will be converted to the Float32 format.
   **/
  class DicomFrameConverter
  {
  private:
    bool    isSigned_;
    bool    isColor_;
    bool    hasRescale_;
    double  rescaleIntercept_;
    double  rescaleSlope_;
    double  defaultWindowCenter_;
    double  defaultWindowWidth_;

    Orthanc::PixelFormat  expectedPixelFormat_;

    void SetDefaultParameters();

  public:
    DicomFrameConverter()
    {
      SetDefaultParameters();
    }

    Orthanc::PixelFormat GetExpectedPixelFormat() const
    {
      return expectedPixelFormat_;
    }

    void ReadParameters(const Orthanc::DicomMap& dicom);

    double GetDefaultWindowCenter() const
    {
      return defaultWindowCenter_;
    }
    
    double GetDefaultWindowWidth() const
    {
      return defaultWindowWidth_;
    }

    void ConvertFrame(std::auto_ptr<Orthanc::ImageAccessor>& source) const;

    void ApplyRescale(Orthanc::ImageAccessor& image,
                      bool useDouble) const;

    double Apply(double x) const;
  };
}
