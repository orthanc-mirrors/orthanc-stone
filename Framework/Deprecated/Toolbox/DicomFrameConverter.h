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

#include <Plugins/Samples/Common/IDicomDataset.h>
#include <Core/Compatibility.h>
#include <Core/DicomFormat/DicomMap.h>
#include <Core/Images/ImageAccessor.h>

#include <memory>

namespace Deprecated
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
    bool    hasDefaultWindow_;
    double  defaultWindowCenter_;
    double  defaultWindowWidth_;
    
    Orthanc::PhotometricInterpretation  photometric_;
    Orthanc::PixelFormat                expectedPixelFormat_;

    void SetDefaultParameters();

  public:
    DicomFrameConverter()
    {
      SetDefaultParameters();
    }

    ~DicomFrameConverter()
    {
      // TODO: check whether this dtor is called or not
      // An MSVC warning explains that declaring an
      // std::unique_ptr with a forward-declared type
      // prevents its dtor from being called. Does not
      // seem an issue here (only POD types inside), but
      // definitely something to keep an eye on.
      (void)0;
    }

    // AM: this is required to serialize/deserialize it
    DicomFrameConverter(
        bool isSigned,
        bool isColor,
        bool hasRescale,
        double rescaleIntercept,
        double rescaleSlope,
        bool hasDefaultWindow,
        double defaultWindowCenter,
        double defaultWindowWidth,
        Orthanc::PhotometricInterpretation photometric,
        Orthanc::PixelFormat expectedPixelFormat
        ):
      isSigned_(isSigned),
      isColor_(isColor),
      hasRescale_(hasRescale),
      rescaleIntercept_(rescaleIntercept),
      rescaleSlope_(rescaleSlope),
      hasDefaultWindow_(hasDefaultWindow),
      defaultWindowCenter_(defaultWindowCenter),
      defaultWindowWidth_(defaultWindowWidth),
      photometric_(photometric),
      expectedPixelFormat_(expectedPixelFormat)
    {}

    void GetParameters(bool& isSigned,
                       bool& isColor,
                       bool& hasRescale,
                       double& rescaleIntercept,
                       double& rescaleSlope,
                       bool& hasDefaultWindow,
                       double& defaultWindowCenter,
                       double& defaultWindowWidth,
                       Orthanc::PhotometricInterpretation& photometric,
                       Orthanc::PixelFormat& expectedPixelFormat) const
    {
      isSigned = isSigned_;
      isColor = isColor_;
      hasRescale = hasRescale_;
      rescaleIntercept = rescaleIntercept_;
      rescaleSlope = rescaleSlope_;
      hasDefaultWindow = hasDefaultWindow_;
      defaultWindowCenter = defaultWindowCenter_;
      defaultWindowWidth = defaultWindowWidth_;
      photometric = photometric_;
      expectedPixelFormat = expectedPixelFormat_;
    }

    Orthanc::PixelFormat GetExpectedPixelFormat() const
    {
      return expectedPixelFormat_;
    }

    Orthanc::PhotometricInterpretation GetPhotometricInterpretation() const
    {
      return photometric_;
    }

    void ReadParameters(const Orthanc::DicomMap& dicom);

    void ReadParameters(const OrthancPlugins::IDicomDataset& dicom);

    bool HasDefaultWindow() const
    {
      return hasDefaultWindow_;
    }
    
    double GetDefaultWindowCenter() const
    {
      return defaultWindowCenter_;
    }
    
    double GetDefaultWindowWidth() const
    {
      return defaultWindowWidth_;
    }

    double GetRescaleIntercept() const
    {
      return rescaleIntercept_;
    }

    double GetRescaleSlope() const
    {
      return rescaleSlope_;
    }

    void ConvertFrameInplace(std::unique_ptr<Orthanc::ImageAccessor>& source) const;

    Orthanc::ImageAccessor* ConvertFrame(const Orthanc::ImageAccessor& source) const;

    void ApplyRescale(Orthanc::ImageAccessor& image,
                      bool useDouble) const;

    double Apply(double x) const;
  };
}
