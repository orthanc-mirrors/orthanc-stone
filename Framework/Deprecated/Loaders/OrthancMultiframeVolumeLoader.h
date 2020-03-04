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

#include "LoaderStateMachine.h"
#include "../../Volumes/DicomVolumeImage.h"
#include "../Volumes/IGeometryProvider.h"

#include <boost/shared_ptr.hpp>

namespace Deprecated
{
  class OrthancMultiframeVolumeLoader :
    public LoaderStateMachine,
    public OrthancStone::IObservable,
    public IGeometryProvider
  {
  private:
    class LoadRTDoseGeometry;
    class LoadGeometry;
    class LoadTransferSyntax;    
    class LoadUncompressedPixelData;

    boost::shared_ptr<OrthancStone::DicomVolumeImage>  volume_;
    std::string                          instanceId_;
    std::string                          transferSyntaxUid_;
    bool                                 pixelDataLoaded_;
    float                                outliersHalfRejectionRate_;
    float                                distributionRawMin_;
    float                                distributionRawMax_;
    float                                computedDistributionMin_;
    float                                computedDistributionMax_;

    const std::string& GetInstanceId() const;

    void ScheduleFrameDownloads();

    void SetTransferSyntax(const std::string& transferSyntax);

    void SetGeometry(const Orthanc::DicomMap& dicom);


    /**
    This method will :
    
    - copy the pixel values from the response to the volume image
    - compute the maximum and minimum value while discarding the
      outliersHalfRejectionRate_ fraction of the outliers from both the start 
      and the end of the distribution.

      In English, this means that, if the volume dataset contains a few extreme
      values very different from the rest (outliers) that we want to get rid of,
      this method allows to do so.

      If you supply 0.005, for instance, it means 1% of the extreme values will
      be rejected (0.5% on each side of the distribution)
    */
    template <typename T>
    void CopyPixelDataAndComputeMinMax(const std::string& pixelData);
      
    /** Service method for CopyPixelDataAndComputeMinMax*/
    template <typename T>
    void CopyPixelDataAndComputeDistribution(
      const std::string& pixelData, 
      std::map<T, uint64_t>& distribution);

    /** Service method for CopyPixelDataAndComputeMinMax*/
    template <typename T>
    void ComputeMinMaxWithOutlierRejection(
      const std::map<T, uint64_t>& distribution);

    void SetUncompressedPixelData(const std::string& pixelData);

    bool HasGeometry() const;
    const OrthancStone::VolumeImageGeometry& GetImageGeometry() const;

  public:
    OrthancMultiframeVolumeLoader(boost::shared_ptr<OrthancStone::DicomVolumeImage> volume,
                                  OrthancStone::IOracle& oracle,
                                  OrthancStone::IObservable& oracleObservable,
                                  float outliersHalfRejectionRate = 0.0005);
    
    virtual ~OrthancMultiframeVolumeLoader();

    bool IsPixelDataLoaded() const
    {
      return pixelDataLoaded_;
    }

    void GetDistributionMinMax
      (float& minValue, float& maxValue) const;

    void GetDistributionMinMaxWithOutliersRejection
      (float& minValue, float& maxValue) const;

    void LoadInstance(const std::string& instanceId);
  };
}
