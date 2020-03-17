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

#include "../../Loaders/IFetchingItemsSorter.h"
#include "../../Loaders/IFetchingStrategy.h"
#include "../../Messages/IObservable.h"
#include "../../Messages/ObserverBase.h"
#include "../../Oracle/GetOrthancImageCommand.h"
#include "../../Oracle/GetOrthancWebViewerJpegCommand.h"
#include "../../Oracle/IOracle.h"
#include "../../Oracle/OrthancRestApiCommand.h"
#include "../../Toolbox/SlicesSorter.h"
#include "../../Volumes/DicomVolumeImage.h"
#include "../../Volumes/IVolumeSlicer.h"

#include "../Volumes/IGeometryProvider.h"


#include <boost/shared_ptr.hpp>

namespace OrthancStone
{
  class ILoadersContext;
}

namespace Deprecated
{
  /**
    This class is used to manage the progressive loading of a volume that
    is stored in a Dicom series.
  */
  class OrthancSeriesVolumeProgressiveLoader : 
    public OrthancStone::ObserverBase<OrthancSeriesVolumeProgressiveLoader>,
    public OrthancStone::IObservable,
    public OrthancStone::IVolumeSlicer,
    public IGeometryProvider
  {
  private:
    static const unsigned int LOW_QUALITY = 0;
    static const unsigned int MIDDLE_QUALITY = 1;
    static const unsigned int BEST_QUALITY = 2;
    
    class ExtractedSlice;
    
    /** Helper class internal to OrthancSeriesVolumeProgressiveLoader */
    class SeriesGeometry : public boost::noncopyable
    {
    private:
      void CheckSlice(size_t index,
                      const OrthancStone::DicomInstanceParameters& reference) const;
    
      void CheckVolume() const;

      void Clear();

      void CheckSliceIndex(size_t index) const;

      std::unique_ptr<OrthancStone::VolumeImageGeometry>     geometry_;
      std::vector<OrthancStone::DicomInstanceParameters*>  slices_;
      std::vector<uint64_t>                  slicesRevision_;

    public:
      ~SeriesGeometry()
      {
        Clear();
      }

      void ComputeGeometry(OrthancStone::SlicesSorter& slices);

      virtual bool HasGeometry() const
      {
        return geometry_.get() != NULL;
      }

      virtual const OrthancStone::VolumeImageGeometry& GetImageGeometry() const;

      const OrthancStone::DicomInstanceParameters& GetSliceParameters(size_t index) const;

      uint64_t GetSliceRevision(size_t index) const;

      void IncrementSliceRevision(size_t index);
    };

    void ScheduleNextSliceDownload();

    void LoadGeometry(const OrthancStone::OrthancRestApiCommand::SuccessMessage& message);

    void SetSliceContent(unsigned int sliceIndex,
                         const Orthanc::ImageAccessor& image,
                         unsigned int quality);

    void LoadBestQualitySliceContent(const OrthancStone::GetOrthancImageCommand::SuccessMessage& message);

    void LoadJpegSliceContent(const OrthancStone::GetOrthancWebViewerJpegCommand::SuccessMessage& message);

    OrthancStone::ILoadersContext&                loadersContext_;
    bool                                          active_;
    unsigned int                                  simultaneousDownloads_;
    SeriesGeometry                                seriesGeometry_;
    boost::shared_ptr<OrthancStone::DicomVolumeImage>           volume_;
    std::unique_ptr<OrthancStone::IFetchingItemsSorter::IFactory> sorter_;
    std::unique_ptr<OrthancStone::IFetchingStrategy>              strategy_;
    std::vector<unsigned int>                     slicesQuality_;
    bool                                          volumeImageReadyInHighQuality_;


    OrthancSeriesVolumeProgressiveLoader(
      OrthancStone::ILoadersContext& loadersContext,
      const boost::shared_ptr<OrthancStone::DicomVolumeImage>& volume);
  
  public:
    ORTHANC_STONE_DEFINE_ORIGIN_MESSAGE(__FILE__, __LINE__, VolumeImageReadyInHighQuality, OrthancSeriesVolumeProgressiveLoader);

    static boost::shared_ptr<OrthancSeriesVolumeProgressiveLoader> Create(
      OrthancStone::ILoadersContext& context,
      const boost::shared_ptr<OrthancStone::DicomVolumeImage>& volume);

    virtual ~OrthancSeriesVolumeProgressiveLoader();

    void SetSimultaneousDownloads(unsigned int count);

    bool IsVolumeImageReadyInHighQuality() const
    {
      return volumeImageReadyInHighQuality_;
    }

    void LoadSeries(const std::string& seriesId);

    /**
    This getter is used by clients that do not receive the geometry through
    subscribing, for instance if they are created or listening only AFTER the
    "geometry loaded" message is broadcast 
    */
    bool HasGeometry() const ORTHANC_OVERRIDE
    {
      return seriesGeometry_.HasGeometry();
    }

    /**
    Same remark as HasGeometry
    */
    const OrthancStone::VolumeImageGeometry& GetImageGeometry() const ORTHANC_OVERRIDE
    {
      return seriesGeometry_.GetImageGeometry();
    }

    /**
    When a slice is requested, the strategy algorithm (that defines the 
    sequence of resources to be loaded from the server) is modified to 
    take into account this request (this is done in the ExtractedSlice ctor)
    */
    virtual IExtractedSlice*
      ExtractSlice(const OrthancStone::CoordinateSystem3D& cuttingPlane) ORTHANC_OVERRIDE;
  };
}
