/**
 * Stone of Orthanc
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include "../Scene2D/LookupTableTextureSceneLayer.h"
#include "../StoneEnumerations.h"
#include "../Toolbox/CoordinateSystem3D.h"
#include "Windowing.h"

#include <IDynamicObject.h>
#include <DicomFormat/DicomImageInformation.h>

namespace OrthancStone
{
  class DicomInstanceParameters :
    public Orthanc::IDynamicObject  /* to be used as a payload to SlicesSorter */
  {
    // This class supersedes the deprecated "DicomFrameConverter"

  private:
    struct Data   // Plain old struct to ease the copy constructor
    {
      std::string         orthancInstanceId_;
      std::string         studyInstanceUid_;
      std::string         seriesInstanceUid_;
      std::string         sopInstanceUid_;
      SopClassUid         sopClassUid_;
      unsigned int        numberOfFrames_;
      unsigned int        width_;
      unsigned int        height_;
      bool                hasSliceThickness_;
      double              sliceThickness_;
      double              pixelSpacingX_;
      double              pixelSpacingY_;
      CoordinateSystem3D  geometry_;
      Vector              frameOffsets_;
      bool                hasRescale_;
      double              rescaleIntercept_;
      double              rescaleSlope_;
      std::vector<Windowing>  windowingPresets_;
      bool                hasIndexInSeries_;
      unsigned int        indexInSeries_;
      std::string         doseUnits_;
      double              doseGridScaling_;
      std::string         frameOfReferenceUid_;
      bool                hasPixelSpacing_;
      bool                hasNumberOfFrames_;
      int32_t             instanceNumber_;
      std::vector<Windowing>  perFrameWindowing_;

      explicit Data(const Orthanc::DicomMap& dicom);
    };

    
    Data                                             data_;
    std::unique_ptr<Orthanc::DicomMap>               tags_;
    std::unique_ptr<Orthanc::DicomImageInformation>  imageInformation_;  // Lazy evaluation

    void InjectSequenceTags(const IDicomDataset& dataset);

  public:
    explicit DicomInstanceParameters(const DicomInstanceParameters& other);

    explicit DicomInstanceParameters(const Orthanc::DicomMap& dicom);

    DicomInstanceParameters* Clone() const
    {
      return new DicomInstanceParameters(*this);
    }

    void SetOrthancInstanceIdentifier(const std::string& id)
    {
      data_.orthancInstanceId_ = id;
    }

    const std::string& GetOrthancInstanceIdentifier() const
    {
      return data_.orthancInstanceId_;
    }

    const Orthanc::DicomMap& GetTags() const
    {
      return *tags_;
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

    unsigned int GetNumberOfFrames() const
    {
      return data_.numberOfFrames_;
    }

    unsigned int GetWidth() const
    {
      return data_.width_;
    }

    unsigned int GetHeight() const
    {
      return data_.height_;
    }

    bool HasSliceThickness() const
    {
      return data_.hasSliceThickness_;
    }

    double GetSliceThickness() const;

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

    // WARNING - Calling this method can throw exception
    const Orthanc::DicomImageInformation& GetImageInformation() const;

    CoordinateSystem3D GetFrameGeometry(unsigned int frame) const;

    bool IsPlaneWithinSlice(unsigned int frame,
                            const CoordinateSystem3D& plane) const;

    bool IsColor() const;

    bool HasRescale() const
    {
      return data_.hasRescale_;
    }

    double GetRescaleIntercept() const;

    double GetRescaleSlope() const;

    Windowing GetFallbackWindowing() const;

    size_t GetWindowingPresetsCount() const;

    Windowing GetWindowingPreset(size_t i) const;

    Windowing GetWindowingPresetsUnion() const;

    Orthanc::PixelFormat GetExpectedPixelFormat() const;

    Orthanc::ImageAccessor* ConvertToFloat(const Orthanc::ImageAccessor& pixelData) const;
    
    TextureBaseSceneLayer* CreateTexture(const Orthanc::ImageAccessor& pixelData) const;

    LookupTableTextureSceneLayer* CreateLookupTableTexture(const Orthanc::ImageAccessor& pixelData) const;

    // NB: According to the DICOM standard, the top-left pixel has
    // (originX, originY) equals to (1,1):
    // https://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_C.9.2.html
    LookupTableTextureSceneLayer* CreateOverlayTexture(int originX,
                                                       int originY,
                                                       const Orthanc::ImageAccessor& overlay) const;

    bool HasIndexInSeries() const
    {
      return data_.hasIndexInSeries_;
    }

    unsigned int GetIndexInSeries() const;

    const std::string& GetDoseUnits() const
    {
      return data_.doseUnits_;
    }

    void SetDoseGridScaling(double value)
    {
      data_.doseGridScaling_ = value;
    }

    double GetDoseGridScaling() const
    {
      return data_.doseGridScaling_;
    }

    void ApplyRescaleAndDoseScaling(Orthanc::ImageAccessor& image,
                                    bool useDouble) const;

    double ApplyRescale(double value) const;

    // Required for RT-DOSE
    bool ComputeFrameOffsetsSpacing(double& target) const;

    const std::string& GetFrameOfReferenceUid() const
    {
      return data_.frameOfReferenceUid_;
    }

    bool HasPixelSpacing() const
    {
      return data_.hasPixelSpacing_;
    }

    void SetPixelSpacing(double pixelSpacingX,
                         double pixelSpacingY);

    void EnrichUsingDicomWeb(const Json::Value& dicomweb);

    bool HasNumberOfFrames() const
    {
      return data_.hasNumberOfFrames_;
    }

    int32_t GetInstanceNumber() const
    {
      return data_.instanceNumber_;
    }

    CoordinateSystem3D GetMultiFrameGeometry() const;

    bool IsReversedFrameOffsets() const;

    bool LookupPerFrameWindowing(Windowing& windowing,
                                 unsigned int frame) const;
  };
}
