/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
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
#include "../Scene2D/TextureBaseSceneLayer.h"
#include "../Toolbox/CoordinateSystem3D.h"

#include <Core/IDynamicObject.h>
#include <Core/DicomFormat/DicomImageInformation.h>

namespace OrthancStone
{
  class DicomInstanceParameters :
    public Orthanc::IDynamicObject  /* to be used as a payload to SlicesSorter */
  {
    // This class supersedes the deprecated "DicomFrameConverter"

  private:
    struct Data   // Struct to ease the copy constructor
    {
      std::string                       orthancInstanceId_;
      std::string                       studyInstanceUid_;
      std::string                       seriesInstanceUid_;
      std::string                       sopInstanceUid_;
      Orthanc::DicomImageInformation    imageInformation_;
      SopClassUid                       sopClassUid_;
      double                            thickness_;
      double                            pixelSpacingX_;
      double                            pixelSpacingY_;
      CoordinateSystem3D                geometry_;
      Vector                            frameOffsets_;
      bool                              isColor_;
      bool                              hasRescale_;
      double                            rescaleIntercept_;
      double                            rescaleSlope_;
      bool                              hasDefaultWindowing_;
      float                             defaultWindowingCenter_;
      float                             defaultWindowingWidth_;
      Orthanc::PixelFormat              expectedPixelFormat_;

      void ComputeDoseOffsets(const Orthanc::DicomMap& dicom);

      Data(const Orthanc::DicomMap& dicom);

      CoordinateSystem3D  GetFrameGeometry(unsigned int frame) const;

      bool IsPlaneWithinSlice(unsigned int frame,
                              const CoordinateSystem3D& plane) const;
      
      void ApplyRescale(Orthanc::ImageAccessor& image,
                        bool useDouble) const;
    };

    
    Data  data_;


  public:
    DicomInstanceParameters(const DicomInstanceParameters& other) :
    data_(other.data_)
    {
    }

    DicomInstanceParameters(const Orthanc::DicomMap& dicom) :
      data_(dicom)
    {
    }

    void SetOrthancInstanceIdentifier(const std::string& id)
    {
      data_.orthancInstanceId_ = id;
    }

    const std::string& GetOrthancInstanceIdentifier() const
    {
      return data_.orthancInstanceId_;
    }

    const Orthanc::DicomImageInformation& GetImageInformation() const
    {
      return data_.imageInformation_;
    }

    const std::string& GetStudyInstanceUid() const
    {
      return data_.studyInstanceUid_;
    }

    const std::string& GetSeriesInstanceUid() const
    {
      return data_.seriesInstanceUid_;
    }

    const std::string& GetSopInstanceUid() const
    {
      return data_.sopInstanceUid_;
    }

    SopClassUid GetSopClassUid() const
    {
      return data_.sopClassUid_;
    }

    double GetThickness() const
    {
      return data_.thickness_;
    }

    double GetPixelSpacingX() const
    {
      return data_.pixelSpacingX_;
    }

    double GetPixelSpacingY() const
    {
      return data_.pixelSpacingY_;
    }

    const CoordinateSystem3D&  GetGeometry() const
    {
      return data_.geometry_;
    }

    CoordinateSystem3D  GetFrameGeometry(unsigned int frame) const
    {
      return data_.GetFrameGeometry(frame);
    }

    bool IsPlaneWithinSlice(unsigned int frame,
                            const CoordinateSystem3D& plane) const
    {
      return data_.IsPlaneWithinSlice(frame, plane);
    }

    bool IsColor() const
    {
      return data_.isColor_;
    }

    bool HasRescale() const
    {
      return data_.hasRescale_;
    }

    double GetRescaleIntercept() const;

    double GetRescaleSlope() const;

    bool HasDefaultWindowing() const
    {
      return data_.hasDefaultWindowing_;
    }

    float GetDefaultWindowingCenter() const;

    float GetDefaultWindowingWidth() const;

    Orthanc::PixelFormat GetExpectedPixelFormat() const
    {
      return data_.expectedPixelFormat_;
    }

    TextureBaseSceneLayer* CreateTexture(const Orthanc::ImageAccessor& pixelData) const;
  };
}